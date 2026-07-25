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

#include "case_encoder.h"

#include <cstddef>
#include <string>
#include <vector>

#include "builder.h"
#include "normalizer.h"
#include "sentencepiece.pb.h"
#include "sentencepiece_processor.h"
#include "sentencepiece_trainer.h"
#include "testharness.h"
#include "util.h"

namespace sentencepiece::normalizer {
namespace {

#define WS "\xe2\x96\x81"

TEST(CaseEncoderTest, SearchCaseSpans) {
  const std::string signature =
      "llUusssUusssUusssllUusssUusssUusss";
  const auto spans = SearchCaseSpans(signature);
  ASSERT_EQ(2, spans.size());
  EXPECT_EQ(2, spans[0].first - signature.data());
  EXPECT_EQ(17, spans[0].second - signature.data());
  EXPECT_EQ(19, spans[1].first - signature.data());
  EXPECT_EQ(34, spans[1].second - signature.data());

  EXPECT_TRUE(SearchCaseSpans("UusssUu").empty());
  EXPECT_TRUE(SearchCaseSpans("lowercase").empty());
}

TEST(CaseEncoderTest, NormalizerOverloadsAndUncasedScripts) {
  NormalizerSpec baseline_spec =
      SentencePieceTrainer::GetNormalizerSpec("nmt_nfkc");
  NormalizerSpec case_spec = baseline_spec;
  case_spec.set_encode_case(true);
  ASSERT_OK(SentencePieceTrainer::PopulateNormalizerSpec(&case_spec, false));

  std::string case_uncaser;
  std::string case_recaser;
  ASSERT_OK(Builder::GetPrecompiledCharsMap("case_uncaser", &case_uncaser));
  ASSERT_OK(Builder::GetPrecompiledCharsMap("case_recaser", &case_recaser));
  EXPECT_FALSE(case_uncaser.empty());
  EXPECT_FALSE(case_recaser.empty());

  TrainerSpec trainer_spec;
  trainer_spec.set_treat_whitespace_as_suffix(true);
  const Normalizer baseline(baseline_spec, trainer_spec);
  const Normalizer case_normalizer(case_spec, trainer_spec);

  const std::string input = "This IS a SHORT PHRASE ABOUT a PhD.";
  const std::string without_offsets = case_normalizer.Normalize(input);
  std::string with_offsets;
  std::vector<size_t> offsets;
  ASSERT_OK(case_normalizer.Normalize(input, &with_offsets, &offsets));
  EXPECT_EQ(without_offsets, with_offsets);
  EXPECT_EQ(with_offsets.size() + 1, offsets.size());

  for (const std::string& uncased : {
           std::string("日本語の文章です。"),
           std::string("هذه جملة باللغة العربية."),
       }) {
    EXPECT_EQ(baseline.Normalize(uncased), case_normalizer.Normalize(uncased));
  }
}

TEST(CaseEncoderTest, DecoderStateMachine) {
  NormalizerSpec decoder_spec;
  decoder_spec.set_decode_case(true);
  decoder_spec.set_add_dummy_prefix(false);
  decoder_spec.set_remove_extra_whitespaces(false);
  decoder_spec.set_escape_whitespaces(false);
  ASSERT_OK(SentencePieceTrainer::PopulateNormalizerSpec(&decoder_spec, true));

  const Normalizer decoder(decoder_spec);
  EXPECT_EQ("This IS a SHORT PHRASE ABOUT a PhD.",
            decoder.Normalize("Tthis Uis a Ashort phrase about La TphTd."));
  EXPECT_EQ("ONE TWO THREE then MixedCase",
            decoder.Normalize("Aone two three Lthen TmixedTcase"));
}

TEST(CaseEncoderTest, LegacyModelCompatibility) {
  SentencePieceProcessor processor;
  ASSERT_OK(processor.Load(
      util::JoinPath(::testing::SrcDir(), "case_encoding_v0.1.94.model")));
  EXPECT_TRUE(processor.model_proto().normalizer_spec().encode_case());
  EXPECT_TRUE(processor.model_proto().denormalizer_spec().decode_case());

  const std::string input = "This IS a SHORT PHRASE ABOUT a PhD.";
  constexpr int kExpectedIds[] = {
      3,  10, 40, 33, 11, 33, 34, 14, 4,  40, 8,
      7,  10, 2,  47, 7,  9,  4,  6,  2,  9,  92,
      8,  16, 10, 2,  31, 34, 3,  47, 3,  18, 5,
  };

  std::vector<int> ids;
  ASSERT_OK(processor.Encode(input, &ids));
  ASSERT_EQ(std::size(kExpectedIds), ids.size());
  for (size_t index = 0; index < ids.size(); ++index) {
    EXPECT_EQ(kExpectedIds[index], ids[index]);
  }

  SentencePieceText decoded;
  ASSERT_OK(processor.Decode(ids, &decoded));
  EXPECT_EQ(input, decoded.text());

  constexpr const char* kExpectedPieces[] = {
      "T",  "t",  "h",  "is" WS, "U",  "is" WS, "a" WS, "A", "s",
      "h",  "o",  "r",  "t",     WS,   "ph",    "r",    "a", "s",
      "e",  WS,   "a",  "b",     "o",  "u",     "t",    WS,  "L",
      "a" WS, "T", "ph", "T",   "d",  "." WS,
  };
  constexpr const char* kExpectedSurfaces[] = {
      "",  "T",  "h",  "is ", "",  "IS ", "a ", "",  "S", "H", "O",
      "R", "T",  " ",  "PH",  "R", "A",   "S",  "E", " ", "A", "B",
      "O", "U",  "T",  "",    " ", "a ",  "",  "Ph", "",  "D", ".",
  };
  constexpr int kExpectedBegins[] = {
      0,  0,  1,  2,  5,  5,  8,  10, 10, 11, 12,
      13, 14, 15, 16, 18, 19, 20, 21, 22, 23, 24,
      25, 26, 27, 28, 28, 29, 31, 31, 33, 33, 34,
  };
  constexpr int kExpectedEnds[] = {
      0,  1,  2,  5,  5,  8,  10, 10, 11, 12, 13,
      14, 15, 16, 18, 19, 20, 21, 22, 23, 24, 25,
      26, 27, 28, 28, 29, 31, 31, 33, 33, 34, 35,
  };

  ASSERT_EQ(std::size(kExpectedPieces), decoded.pieces_size());
  std::string joined_surfaces;
  for (int index = 0; index < decoded.pieces_size(); ++index) {
    const auto& piece = decoded.pieces(index);
    EXPECT_EQ(kExpectedPieces[index], piece.piece());
    EXPECT_EQ(kExpectedSurfaces[index], piece.surface());
    EXPECT_EQ(kExpectedBegins[index], piece.begin());
    EXPECT_EQ(kExpectedEnds[index], piece.end());
    joined_surfaces += piece.surface();
  }
  EXPECT_EQ(decoded.text(), joined_surfaces);
}

TEST(CaseEncoderTest, TrainAndRoundTripMixedScripts) {
  const std::vector<std::string> samples = {
      "This IS a SHORT PHRASE ABOUT a PhD.",
      "ΑΥΤΟ ΕΙΝΑΙ ΕΝΑ ΕΛΛΗΝΙΚΟ ΚΕΙΜΕΝΟ.",
      "ЭТО ТЕКСТ НА РУССКОМ ЯЗЫКЕ.",
      "ԱՅՍ ՀԱՅԵՐԵՆ ՏԵՔՍՏ Է։",
      "ႧႤႵႱႲႨ ႵႠႰႧႳႪႠႣ.",
      "日本語の文章です。",
      "هذه جملة باللغة العربية.",
      "Numbers 123 and emoji 😀 DO NOT carry case.",
  };

  std::vector<std::string> training_samples;
  for (int repeat = 0; repeat < 4; ++repeat) {
    training_samples.insert(training_samples.end(), samples.begin(),
                            samples.end());
  }

  std::string serialized_model;
  ASSERT_OK(SentencePieceTrainer::Train(
      "--model_type=unigram --vocab_size=512 --hard_vocab_limit=false "
      "--character_coverage=1.0 --byte_fallback=true "
      "--input_sentence_size=0 --shuffle_input_sentence=false "
      "--num_threads=1 --encode_unicode_case=true "
      "--treat_whitespace_as_suffix=true --bos_id=-1 --eos_id=0 --unk_id=1",
      training_samples, &serialized_model));

  SentencePieceProcessor processor;
  ASSERT_OK(processor.LoadFromSerializedProto(serialized_model));
  EXPECT_TRUE(processor.model_proto().normalizer_spec().encode_case());
  EXPECT_TRUE(processor.model_proto().denormalizer_spec().decode_case());

  for (const auto& sample : samples) {
    std::vector<int> ids;
    ASSERT_OK(processor.Encode(sample, &ids));
    std::string decoded;
    ASSERT_OK(processor.Decode(ids, &decoded));
    EXPECT_EQ(sample, decoded);
  }
}

#undef WS

}  // namespace
}  // namespace sentencepiece::normalizer
