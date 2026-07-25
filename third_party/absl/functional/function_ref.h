// C++17 FunctionRef compatibility for standalone SentencePiece.
#ifndef ABSL_FUNCTIONAL_FUNCTION_REF_H_
#define ABSL_FUNCTIONAL_FUNCTION_REF_H_

#include <functional>

#include "third_party/absl/base/attributes.h"

namespace absl {
template <class Signature>
using FunctionRef = std::function<Signature>;
}  // namespace absl

#endif  // ABSL_FUNCTIONAL_FUNCTION_REF_H_
