// C++17 AnyInvocable compatibility for standalone SentencePiece.
#ifndef ABSL_FUNCTIONAL_ANY_INVOCABLE_H_
#define ABSL_FUNCTIONAL_ANY_INVOCABLE_H_

#include <functional>

namespace absl {
template <class Signature>
using AnyInvocable = std::function<Signature>;
}  // namespace absl

#endif  // ABSL_FUNCTIONAL_ANY_INVOCABLE_H_
