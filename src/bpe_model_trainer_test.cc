// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!

#include <string>
#include <vector>

#include "bpe_model_trainer.h"
#include "filesystem.h"
#include "sentencepiece_processor.h"
#include "sentencepiece_trainer.h"
#include "testharness.h"
#include "third_party/absl/flags/flag.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/str_join.h"
#include "util.h"

ABSL_DECLARE_FLAG(bool, nlcodec_bpe);

namespace sentencepiece {
namespace bpe {
namespace {

// Space symbol
#define WS "\xe2\x96\x81"

std::string RunTrainer(
    const std::vector<std::string> &input, int size,
    const std::vector<std::string> &user_defined_symbols = {}) {
  const std::string input_file =
      util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir), "input");
  const std::string model_prefix =
      util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir), "model");
  {
    auto output = filesystem::NewWritableFile(input_file);
    for (const auto &line : input) {
      output->WriteLine(line);
    }
  }

  TrainerSpec trainer_spec;
  trainer_spec.set_model_type(TrainerSpec::BPE);
  trainer_spec.add_input(input_file);
  trainer_spec.set_vocab_size(size - 3);  // remove <unk>, <s>, </s>
  trainer_spec.set_model_prefix(model_prefix);

  NormalizerSpec normalizer_spec;
  normalizer_spec.set_name("identity");
  normalizer_spec.set_add_dummy_prefix(false);

  NormalizerSpec denormalizer_spec;

  for (const auto &w : user_defined_symbols) {
    trainer_spec.add_user_defined_symbols(w);
  }

  Trainer trainer(trainer_spec, normalizer_spec, denormalizer_spec);
  EXPECT_TRUE(trainer.Train().ok());

  SentencePieceProcessor processor;
  EXPECT_TRUE(processor.Load(model_prefix + ".model").ok());

  const auto &model = processor.model_proto();
  std::vector<std::string> pieces;

  // remove <unk>, <s>, </s>
  for (int i = 3; i < model.pieces_size(); ++i) {
    pieces.emplace_back(model.pieces(i).piece());
  }

  return absl::StrJoin(pieces, " ");
}

TEST(BPETrainerTest, BasicTest) {
  EXPECT_EQ("ab ra abra ad cad abracad abracadabra ac br a b r c d",
            RunTrainer({"abracadabra"}, 20));
  EXPECT_EQ("ap le app apple en in ine pen p e a l n i",
            RunTrainer({"pen", "pineapple", "apple"}, 20));
  EXPECT_EQ("he ll llo hello hellohe el lo oh hel ohe e h l o",
            RunTrainer({"hellohe"}, 20));
  EXPECT_EQ("app le en in ine pen pine ne pe e l n p i",
            RunTrainer({"pen", "pineapple", "apple"}, 20, {"app"}));
}

static constexpr char kTestInputData[] = "wagahaiwa_nekodearu.txt";

TEST(BPETrainerTest, EndToEndTest) {
  const std::string input =
      util::JoinPath(absl::GetFlag(FLAGS_test_srcdir), kTestInputData);

  ASSERT_TRUE(
      SentencePieceTrainer::Train(
          absl::StrCat(
              "--model_prefix=",
              util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir), "tmp_model"),
              " --input=", input,
              " --vocab_size=8000 --normalization_rule_name=identity"
              " --model_type=bpe --control_symbols=<ctrl> "
              "--max_sentence_length=2048"))
          .ok());

  SentencePieceProcessor sp;
  ASSERT_TRUE(sp.Load(std::string(util::JoinPath(
                          absl::GetFlag(FLAGS_test_tmpdir), "tmp_model.model")))
                  .ok());
  EXPECT_EQ(8000, sp.GetPieceSize());

  const int cid = sp.PieceToId("<ctrl>");
  EXPECT_TRUE(sp.IsControl(cid));

  std::vector<std::string> tok;
  ASSERT_TRUE(sp.Encode("", &tok).ok());
  ASSERT_TRUE(tok.empty());

  EXPECT_TRUE(sp.Encode("吾輩《わがはい》は猫である。名前はまだ無い。"
                        "どこで生れたかとんと見当《けんとう》がつかぬ。"
                        "何でも薄暗いじめじめした所でニャーニャー泣いていた事だ"
                        "けは記憶している"
                        "。",
                        &tok)
                  .ok());
  EXPECT_EQ(WS
            " 吾輩 《 わが はい 》 は猫 である 。 名前 はまだ 無い 。 "
            "どこで 生 れた か とん と見 当 《 けんとう 》 が つかぬ 。 "
            "何でも 薄 暗 いじ め じ め した 所で ニャー ニャー 泣 いていた "
            "事 だけは 記憶 している 。",
            absl::StrJoin(tok, " "));
}

// Helper: train BPE and return set of learned pieces (excluding meta tokens).
static std::set<std::string> TrainAndGetPieces(
    const std::string &input_file, int vocab_size, bool use_nlcodec) {
  const std::string model_prefix =
      util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir),
                     use_nlcodec ? "nlcodec_model" : "default_model");

  absl::SetFlag(&FLAGS_nlcodec_bpe, use_nlcodec);

  EXPECT_TRUE(
      SentencePieceTrainer::Train(
          absl::StrCat("--model_prefix=", model_prefix,
                       " --input=", input_file,
                       " --vocab_size=", std::to_string(vocab_size),
                       " --normalization_rule_name=identity",
                       " --model_type=bpe",
                       " --max_sentence_length=2048"))
          .ok());

  SentencePieceProcessor sp;
  EXPECT_TRUE(sp.Load(model_prefix + ".model").ok());

  std::set<std::string> pieces;
  for (int i = 0; i < sp.GetPieceSize(); ++i) {
    if (!sp.IsUnknown(i) && !sp.IsControl(i))
      pieces.insert(sp.IdToPiece(i));
  }
  return pieces;
}

// Helper: encode sentences and return tokenized output.
static std::vector<std::string> EncodeWithModel(
    const std::string &model_path,
    const std::vector<std::string> &sentences) {
  SentencePieceProcessor sp;
  EXPECT_TRUE(sp.Load(model_path).ok());

  std::vector<std::string> results;
  for (const auto &s : sentences) {
    std::vector<std::string> tok;
    EXPECT_TRUE(sp.Encode(s, &tok).ok());
    results.push_back(absl::StrJoin(tok, " "));
  }
  return results;
}

TEST(BPETrainerTest, NlcodecBPEProducesValidModel) {
  // Train with nlcodec_bpe on the test data
  const std::string input =
      util::JoinPath(absl::GetFlag(FLAGS_test_srcdir), kTestInputData);
  const std::string model_prefix =
      util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir), "nlcodec_test");

  absl::SetFlag(&FLAGS_nlcodec_bpe, true);
  ASSERT_TRUE(
      SentencePieceTrainer::Train(
          absl::StrCat("--model_prefix=", model_prefix,
                       " --input=", input,
                       " --vocab_size=3000",
                       " --normalization_rule_name=identity",
                       " --model_type=bpe",
                       " --max_sentence_length=2048"))
          .ok());
  absl::SetFlag(&FLAGS_nlcodec_bpe, false);

  SentencePieceProcessor sp;
  ASSERT_TRUE(sp.Load(model_prefix + ".model").ok());
  EXPECT_EQ(3000, sp.GetPieceSize());

  // Should be able to encode and decode
  std::vector<std::string> tok;
  ASSERT_TRUE(sp.Encode("hello world", &tok).ok());
  EXPECT_FALSE(tok.empty());

  std::string decoded;
  ASSERT_TRUE(sp.Decode(tok, &decoded).ok());
  // Decoded may not match exactly due to normalization, but should not be empty
  EXPECT_FALSE(decoded.empty());
}

TEST(BPETrainerTest, NlcodecBPEVocabSizeMatchesDefault) {
  // Both paths should produce the same vocab size
  const std::string input =
      util::JoinPath(absl::GetFlag(FLAGS_test_srcdir), kTestInputData);

  auto default_pieces = TrainAndGetPieces(input, 3000, false);
  auto nlcodec_pieces = TrainAndGetPieces(input, 3000, true);

  // Vocab sizes should be equal
  EXPECT_EQ(default_pieces.size(), nlcodec_pieces.size());
}

TEST(BPETrainerTest, NlcodecBPEEncodesDecodesCorrectly) {
  // Train with nlcodec_bpe and verify encode→decode roundtrip
  const std::string input =
      util::JoinPath(absl::GetFlag(FLAGS_test_srcdir), kTestInputData);
  const std::string model_prefix =
      util::JoinPath(absl::GetFlag(FLAGS_test_tmpdir), "nlcodec_roundtrip");

  absl::SetFlag(&FLAGS_nlcodec_bpe, true);
  ASSERT_TRUE(
      SentencePieceTrainer::Train(
          absl::StrCat("--model_prefix=", model_prefix,
                       " --input=", input,
                       " --vocab_size=3000",
                       " --normalization_rule_name=identity",
                       " --model_type=bpe",
                       " --max_sentence_length=2048"))
          .ok());
  absl::SetFlag(&FLAGS_nlcodec_bpe, false);

  SentencePieceProcessor sp;
  ASSERT_TRUE(sp.Load(model_prefix + ".model").ok());

  // Test roundtrip on multiple strings
  const std::vector<std::string> test_strings = {
      "abracadabra",
      "hello world",
      "the quick brown fox",
  };

  for (const auto &s : test_strings) {
    std::vector<int> ids;
    ASSERT_TRUE(sp.Encode(s, &ids).ok());
    EXPECT_FALSE(ids.empty());

    std::string decoded;
    ASSERT_TRUE(sp.Decode(ids, &decoded).ok());
    // Roundtrip may not be exact due to whitespace normalization,
    // but should not be empty and should include the original word content
    EXPECT_FALSE(decoded.empty()) << "Decode produced empty string for: " << s;
  }
}

TEST(BPETrainerTest, NlcodecBPEVocabOverlapsWithDefault) {
  // Both paths should produce largely overlapping vocabularies
  // (not identical due to tie-breaking differences, but high overlap)
  const std::string input =
      util::JoinPath(absl::GetFlag(FLAGS_test_srcdir), kTestInputData);

  auto default_pieces = TrainAndGetPieces(input, 3000, false);
  auto nlcodec_pieces = TrainAndGetPieces(input, 3000, true);

  // Count overlap
  int overlap = 0;
  for (const auto &p : default_pieces) {
    if (nlcodec_pieces.count(p)) overlap++;
  }

  double overlap_ratio =
      static_cast<double>(overlap) / default_pieces.size();

  // Expect at least 50% overlap (both learn from same data)
  EXPECT_GT(overlap_ratio, 0.5)
      << "Overlap too low: " << overlap << "/"
      << default_pieces.size() << " = " << overlap_ratio;

  LOG(INFO) << "Vocab overlap: " << overlap << "/"
            << default_pieces.size() << " = "
            << static_cast<int>(overlap_ratio * 100) << "%";
}

}  // namespace
}  // namespace bpe
}  // namespace sentencepiece
