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

std::unique_ptr<CaseEncoder> CaseEncoder::Create(bool encodeCase, bool decodeCase) {
  if(encodeCase && decodeCase) {
    LOG(ERROR) << "Cannot set both encodeCase=true and decodeCase=true";
    return nullptr;
  } else if(encodeCase) {
    return std::unique_ptr<CaseEncoder>(new UpperCaseEncoder());
  } else if(decodeCase) {
    return std::unique_ptr<CaseEncoder>(new UpperCaseDecoder());
  } else {
    return nullptr;
  }
}

}
}