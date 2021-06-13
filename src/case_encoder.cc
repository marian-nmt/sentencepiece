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

namespace sentencepiece {
namespace normalizer {

std::unique_ptr<CaseEncoder> CaseEncoder::Create(bool encodeCase, bool decodeCase, bool removeExtraWhiteSpace) {
  if(encodeCase && decodeCase) {
    LOG(ERROR) << "Cannot set both encodeCase=true and decodeCase=true";
    return nullptr;
  } else if(encodeCase) {
    return std::unique_ptr<CaseEncoder>(new UpperCaseEncoder(removeExtraWhiteSpace));
  } else if(decodeCase) {
    return std::unique_ptr<CaseEncoder>(new UpperCaseDecoder());
  } else {
    return nullptr;
  }
}

constexpr size_t npos = -1;

constexpr int fsa[][4] = {
  {  7, -1, -1, -1},
  { -1,  4,  5,  1},
  {  3,  2, 14, -1},
  { -1, -1, -1,  1},
  {  3,  4,  5, -1},
  { -1, -1,  6, -1},
  { -1, -1,  4, -1},
  { -1, -1, -1,  8},
  { -1,  9, 10,  8},
  { 11,  9, 10, -1},
  { -1, -1, 12, -1},
  { -1, -1, -1, 13},
  { -1, -1,  9, -1},
  { -1,  2, 14, 13},
  { -1, -1, 15, -1},
  { -1, -1,  2, -1}
};

constexpr bool accept[16] = {
  false, false, false, false,
  true,  false, false, false,
  false, false, false, false,
  false, false, false, false
};

constexpr int alphabet(char c) {
  switch (c) {
    case 'U': return 0;
    case 'p': return 1;
    case '$': return 1;
    case 's': return 2;
    case 'u': return 3;
    default: return -1;
  };
}

constexpr int delta(int state, char c) {
  int a = alphabet(c);
  return a != -1 ? fsa[state][a] : -1;
}

size_t searchLongestSuffix(const char* data, size_t length) {
  size_t found = -1;
  int state = 0;
  
  if(accept[state]) 
    found = 0;

  for(size_t i = 0; i < length; ++i) {
    state = delta(state, data[i]);
    
    if(state == -1)
      return found;

    if(accept[state])
      found = i + 1;
  }

  state = delta(state, '$');
  if(state != -1 && accept[state])
      found = length;

  return found;
}

std::vector<std::pair<const char*, const char*>> search(const std::string& input) {
  std::vector<std::pair<const char*, const char*>> results;
  for(size_t i = 0; i < input.length(); ++i) {
    size_t found = searchLongestSuffix(input.data() + i, input.length() - i);
    if(found != npos) {
      results.emplace_back(input.data() + i, input.data() + i + found);
      i += found - 1;
    }
  }
  return results;
}

}
}