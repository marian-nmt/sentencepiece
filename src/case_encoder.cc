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

#include <iterator>
#include <utility>

#include "common.h"

namespace sentencepiece {
namespace normalizer {
namespace {

constexpr size_t kNotFound = static_cast<size_t>(-1);
constexpr int kSinkState = -1;

// Implements Uu+(sss|p|$)+Uu+(sss|p|$)+(Uu+(sss|p|$)+)+.
constexpr int kTransitions[][4] = {
    {7, kSinkState, kSinkState, kSinkState},
    {kSinkState, 4, 5, 1},
    {3, 2, 14, kSinkState},
    {kSinkState, kSinkState, kSinkState, 1},
    {3, 4, 5, kSinkState},
    {kSinkState, kSinkState, 6, kSinkState},
    {kSinkState, kSinkState, 4, kSinkState},
    {kSinkState, kSinkState, kSinkState, 8},
    {kSinkState, 9, 10, 8},
    {11, 9, 10, kSinkState},
    {kSinkState, kSinkState, 12, kSinkState},
    {kSinkState, kSinkState, kSinkState, 13},
    {kSinkState, kSinkState, 9, kSinkState},
    {kSinkState, 2, 14, 13},
    {kSinkState, kSinkState, 15, kSinkState},
    {kSinkState, kSinkState, 2, kSinkState},
};

constexpr bool kAcceptingStates[] = {
    false, false, false, false, true,  false, false, false,
    false, false, false, false, false, false, false, false,
};

int AlphabetIndex(char value) {
  switch (value) {
    case 'U':
      return 0;
    case 'p':
    case '$':
      return 1;
    case 's':
      return 2;
    case 'u':
      return 3;
    default:
      return -1;
  }
}

int NextState(int state, char value) {
  const int alphabet_index = AlphabetIndex(value);
  return alphabet_index == -1 ? kSinkState
                              : kTransitions[state][alphabet_index];
}

size_t SearchLongestCaseSpan(const char* data, size_t length) {
  size_t found = kNotFound;
  int state = 0;
  if (kAcceptingStates[state]) found = 0;

  for (size_t index = 0; index < length; ++index) {
    state = NextState(state, data[index]);
    if (state == kSinkState) return found;
    if (kAcceptingStates[state]) found = index + 1;
  }

  state = NextState(state, '$');
  if (state != kSinkState && kAcceptingStates[state]) found = length;
  return found;
}

}  // namespace

std::unique_ptr<CaseEncoder> CaseEncoder::Create(
    bool encode_case, bool decode_case, bool remove_extra_whitespace) {
  if (encode_case && decode_case) {
    LOG(ERROR) << "Cannot set both encode_case=true and decode_case=true";
    return nullptr;
  }
  if (encode_case) {
    return std::make_unique<UpperCaseEncoder>(remove_extra_whitespace);
  }
  if (decode_case) return std::make_unique<UpperCaseDecoder>();
  return nullptr;
}

std::vector<std::pair<const char*, const char*>> SearchCaseSpans(
    const std::string& input) {
  std::vector<std::pair<const char*, const char*>> results;
  for (size_t index = 0; index < input.size(); ++index) {
    const size_t found =
        SearchLongestCaseSpan(input.data() + index, input.size() - index);
    if (found != kNotFound) {
      results.emplace_back(input.data() + index,
                           input.data() + index + found);
      index += found - 1;
    }
  }
  return results;
}

void UpperCaseEncoder::Buffer(absl::string_view normalized, int consumed) {
  const size_t previous_size = buffer_.size();
  buffer_.append(normalized.data(), normalized.size());
  buffer_queue_.emplace_back(buffer_.substr(previous_size, normalized.size()),
                             consumed);
}

std::pair<absl::string_view, int> UpperCaseEncoder::NormalizePrefix(
    absl::string_view original_input) {
  if (dump_buffer_from_ >= 0 &&
      static_cast<size_t>(dump_buffer_from_) < buffer_queue_.size()) {
    const auto& buffered = buffer_queue_[dump_buffer_from_++];
    return {buffered.first, buffered.second};
  }

  if (dump_buffer_from_ > -1) {
    dump_buffer_from_ = -1;
    buffer_queue_.clear();
    return {};
  }

  const absl::string_view input = original_input.substr(offset_);
  auto result = CaseEncoder::NormalizePrefix(input);
  absl::string_view normalized = result.first;
  const int consumed = result.second;
  const bool last = input.size() == static_cast<size_t>(consumed);

  if (normalized.empty()) {
    if (state_ == 0 && offset_ == 0) return result;
    offset_ += consumed;
    if (last) dump_buffer_from_ = 0;
    return {};
  }

  auto defer = [this](int deferred_consumed) {
    offset_ += deferred_consumed;
    return std::pair<absl::string_view, int>();
  };

  const bool is_uppercase = normalized.front() == kCaseUppercase;
  const bool is_punctuation = normalized.front() == kCasePunctuation;
  const bool is_space = normalized.front() == ' ';

  if (state_ == 0) {
    buffer_.clear();
    buffer_queue_.clear();
    offset_ = 0;
  }

  if (is_uppercase) {
    if (state_ == 0) {
      Buffer(normalized, consumed);
      buffer_.front() = kCaseTitlecase;
      buffer_queue_.front().first.front() = kCaseTitlecase;
      state_ = 1;
      result = defer(consumed);
      signature_.push_back('U');
      signature_.append(normalized.size() - 1, 'u');
    } else if (state_ == 1 || state_ == 2) {
      if (state_ == 1) ++spans_;
      normalized.remove_prefix(1);
      Buffer(normalized, consumed);
      buffer_.front() = kCaseUppercase;
      buffer_queue_.front().first.front() = kCaseUppercase;
      state_ = 2;
      result = defer(consumed);
      signature_.append(normalized.size(), 'u');
    }

    if (last) {
      dump_buffer_from_ = 0;
      return defer(0);
    }
  } else {
    if (is_punctuation) {
      if (state_ == 1) ++spans_;
      normalized.remove_prefix(1);
      signature_.append(normalized.size(), 'p');
    } else if (state_ == 2 && !is_space) {
      spans_ = 0;
      Buffer(std::string(1, kCaseLowercase), 0);
      signature_.push_back('L');
      signature_.append(normalized.size(), 'l');
    } else if (is_space) {
      if (state_ == 1) ++spans_;
      if (!remove_extra_whitespace_ || signature_.empty() ||
          signature_.back() != 's') {
        signature_.append("sss");
      }
    } else {
      spans_ = 0;
      signature_.append(normalized.size(), 'l');
    }

    if (!buffer_.empty()) {
      Buffer(normalized, consumed);
      offset_ = 0;
      dump_buffer_from_ = 0;
      state_ = 0;
      return defer(0);
    }

    result.first = normalized;
    state_ = 0;
  }

  if (spans_ >= 3) seen_three_spans_ = true;
  return result;
}

void UpperCaseEncoder::Postprocess(std::string* normalized,
                                   std::vector<size_t>* norm_to_orig) {
  if (!seen_three_spans_) return;

  std::string output;
  output.reserve(normalized->size());
  std::vector<size_t> output_offsets;
  output_offsets.reserve(norm_to_orig->size());

  const char* signature_iterator = signature_.data();
  auto normalized_iterator = normalized->cbegin();
  auto offset_iterator = norm_to_orig->cbegin();

  for (const auto& span : SearchCaseSpans(signature_)) {
    const size_t prefix_length =
        std::distance(signature_iterator, span.first);
    output.insert(output.end(), normalized_iterator,
                  normalized_iterator + prefix_length);
    output_offsets.insert(output_offsets.end(), offset_iterator,
                          offset_iterator + prefix_length);

    signature_iterator += prefix_length;
    normalized_iterator += prefix_length;
    offset_iterator += prefix_length;
    output.push_back(kCaseAllUppercase);
    output_offsets.push_back(*offset_iterator);

    while (signature_iterator != span.second) {
      if (*signature_iterator == kCaseUppercase) {
        ++signature_iterator;
        ++normalized_iterator;
        ++offset_iterator;
      }
      ++signature_iterator;
      output.push_back(*normalized_iterator++);
      output_offsets.push_back(*offset_iterator++);
    }
    if (signature_iterator != signature_.data() + signature_.size() &&
        *signature_iterator != kCaseUppercase) {
      output.push_back(kCaseLowercase);
      output_offsets.push_back(*offset_iterator);
    }
  }

  output.insert(output.end(), normalized_iterator, normalized->cend());
  output_offsets.insert(output_offsets.end(), offset_iterator,
                        norm_to_orig->cend());
  normalized->swap(output);
  norm_to_orig->swap(output_offsets);
}

std::pair<absl::string_view, int> UpperCaseDecoder::NormalizePrefix(
    absl::string_view input) {
  if (!buffer_) {
    buffer_ = std::make_unique<std::string>(input.data(), input.size());
    input_ = *buffer_;
  }
  if (input_.empty()) return {};

  auto replace_input_front = [this](char value) {
    buffer_->at(input_.data() - buffer_->data()) = value;
  };

  if (input_.front() == kCaseAllUppercase) {
    replace_input_front(kCaseUppercase);
    all_uppercase_ = true;
  } else if (input_.front() == kCaseTitlecase ||
             input_.front() == kCaseLowercase) {
    all_uppercase_ = false;
  }

  auto result = CaseEncoder::NormalizePrefix(input_);
  const int consumed = result.second;
  if (result.first.empty() || consumed == 0) return result;

  if (input_.front() == kCaseUppercase) {
    if (state_ == 0) {
      input_.remove_prefix(consumed - 1);
      replace_input_front(kCaseUppercase);
      state_ = 1;
    } else if (state_ == 1) {
      if (consumed > 1) {
        input_.remove_prefix(consumed - 1);
        replace_input_front(kCaseUppercase);
        result.second = consumed - 1;
      } else {
        input_.remove_prefix(consumed);
        result.first.remove_prefix(1);
        result.second = 0;
        state_ = 0;
      }
    }
  } else if (input_.front() == kCaseLowercase) {
    input_.remove_prefix(consumed);
    result.first.remove_prefix(1);
    state_ = 0;
  } else if (all_uppercase_) {
    result.first = absl::string_view(input.data(), result.first.size());
    input_.remove_prefix(consumed - 1);
    replace_input_front(kCaseUppercase);
    state_ = 1;
  } else {
    input_.remove_prefix(consumed);
    state_ = 0;
  }

  return result;
}

}  // namespace normalizer
}  // namespace sentencepiece