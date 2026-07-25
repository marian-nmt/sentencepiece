//
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef ABSL_STRINGS_NUMBERS_H_
#define ABSL_STRINGS_NUMBERS_H_

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

#include "third_party/absl/strings/string_view.h"

namespace absl {

// TODO(taku): Re-implement this, as it is slow.
template <typename T>
inline bool SimpleAtoi(absl::string_view s, T *result) {
  std::stringstream ss;
  return (ss << s.data() && ss >> *result);
}

inline bool SimpleAtod(absl::string_view input, double* result) {
  const std::string text(input);
  char* end = nullptr;
  errno = 0;
  const double value = std::strtod(text.c_str(), &end);
  if (errno != 0 || end != text.c_str() + text.size()) return false;
  *result = value;
  return true;
}

inline bool SimpleAtob(absl::string_view input, bool* result) {
  std::string text(input);
  for (char& value : text) {
    if (value >= 'A' && value <= 'Z') value += 'a' - 'A';
  }
  if (text == "true" || text == "1") {
    *result = true;
    return true;
  }
  if (text == "false" || text == "0") {
    *result = false;
    return true;
  }
  return false;
}

namespace numbers_internal {

template <typename T>
inline bool safe_strtoi_base(absl::string_view input, T* result, int base) {
  static_assert(std::is_integral_v<T>, "T must be an integral type");
  const std::string text(input);
  char* end = nullptr;
  errno = 0;
  if constexpr (std::is_signed_v<T>) {
    const long long value = std::strtoll(text.c_str(), &end, base);
    if (errno != 0 || end != text.c_str() + text.size() ||
        value < std::numeric_limits<T>::min() ||
        value > std::numeric_limits<T>::max()) {
      return false;
    }
    *result = static_cast<T>(value);
  } else {
    if (!text.empty() && text.front() == '-') return false;
    const unsigned long long value = std::strtoull(text.c_str(), &end, base);
    if (errno != 0 || end != text.c_str() + text.size() ||
        value > std::numeric_limits<T>::max()) {
      return false;
    }
    *result = static_cast<T>(value);
  }
  return true;
}

}  // namespace numbers_internal

}  // namespace absl
#endif  // ABSL_STRINGS_NUMBERS_H_
