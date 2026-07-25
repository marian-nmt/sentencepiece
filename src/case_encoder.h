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

#ifndef NORMALIZER_CASE_ENCODER_H_
#define NORMALIZER_CASE_ENCODER_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "third_party/absl/strings/string_view.h"

namespace sentencepiece {
namespace normalizer {

constexpr char kCaseUppercase = 'U';
constexpr char kCaseAllUppercase = 'A';
constexpr char kCaseTitlecase = 'T';
constexpr char kCaseLowercase = 'L';
constexpr char kCasePunctuation = 'P';

std::vector<std::pair<const char*, const char*>> SearchCaseSpans(
    const std::string& input);

class CaseEncoder {
 public:
  using Normalizer = std::function<std::pair<absl::string_view, int>(
      absl::string_view)>;

  virtual ~CaseEncoder() = default;

  virtual std::pair<absl::string_view, int> NormalizePrefix(
      absl::string_view input) {
    return normalizer_(input);
  }

  void SetNormalizer(Normalizer normalizer) {
    normalizer_ = std::move(normalizer);
  }

  virtual void Postprocess(std::string* normalized,
                           std::vector<size_t>* norm_to_orig) {}

  static std::unique_ptr<CaseEncoder> Create(bool encode_case,
                                              bool decode_case,
                                              bool remove_extra_whitespace);

 protected:
  Normalizer normalizer_;
};

class UpperCaseEncoder : public CaseEncoder {
 public:
  explicit UpperCaseEncoder(bool remove_extra_whitespace)
      : remove_extra_whitespace_(remove_extra_whitespace) {}

  std::pair<absl::string_view, int> NormalizePrefix(
      absl::string_view input) override;
  void Postprocess(std::string* normalized,
                   std::vector<size_t>* norm_to_orig) override;

 private:
  void Buffer(absl::string_view normalized, int consumed);

  std::string buffer_;
  std::string signature_;
  size_t offset_ = 0;
  std::vector<std::pair<std::string, int>> buffer_queue_;
  int dump_buffer_from_ = -1;
  int state_ = 0;
  size_t spans_ = 0;
  bool seen_three_spans_ = false;
  bool remove_extra_whitespace_ = false;
};

class UpperCaseDecoder : public CaseEncoder {
 public:
  std::pair<absl::string_view, int> NormalizePrefix(
      absl::string_view input) override;

 private:
  enum class State {
    kNormal,
    kUppercaseRun,
  };

  std::unique_ptr<std::string> buffer_;
  absl::string_view input_;
  State state_ = State::kNormal;
  bool in_all_uppercase_span_ = false;
};

}  // namespace normalizer
}  // namespace sentencepiece

#endif  // NORMALIZER_CASE_ENCODER_H_