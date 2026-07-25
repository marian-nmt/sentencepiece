// C++17 FixedArray compatibility for standalone SentencePiece.
#ifndef ABSL_CONTAINER_FIXED_ARRAY_H_
#define ABSL_CONTAINER_FIXED_ARRAY_H_

#include <cstddef>
#include <vector>

namespace absl {
template <class T, std::size_t InlineElements = 0>
using FixedArray = std::vector<T>;
}  // namespace absl

#endif  // ABSL_CONTAINER_FIXED_ARRAY_H_
