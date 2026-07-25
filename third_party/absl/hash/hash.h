// C++17 hash compatibility for standalone SentencePiece.
#ifndef ABSL_HASH_HASH_H_
#define ABSL_HASH_HASH_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace absl {
namespace hash_internal {

inline std::size_t Mix(std::size_t seed, std::size_t value) {
  return seed ^ (value + static_cast<std::size_t>(0x9e3779b97f4a7c15ULL) +
                 (seed << 6) + (seed >> 2));
}

inline std::uint64_t MixPair(std::uint64_t first, std::uint64_t second) {
  std::uint64_t middle = 0xe08c1d668b756f82ULL;
  first -= middle;
  first -= second;
  first ^= second >> 43;
  middle -= second;
  middle -= first;
  middle ^= first << 9;
  second -= first;
  second -= middle;
  second ^= middle >> 8;
  first -= middle;
  first -= second;
  first ^= second >> 38;
  middle -= second;
  middle -= first;
  middle ^= first << 23;
  second -= first;
  second -= middle;
  second ^= middle >> 5;
  first -= middle;
  first -= second;
  first ^= second >> 35;
  middle -= second;
  middle -= first;
  middle ^= first << 49;
  second -= first;
  second -= middle;
  second ^= middle >> 11;
  first -= middle;
  first -= second;
  first ^= second >> 12;
  middle -= second;
  middle -= first;
  middle ^= first << 18;
  second -= first;
  second -= middle;
  second ^= middle >> 22;
  return second;
}

}  // namespace hash_internal

inline std::size_t HashOf(std::uint64_t first, std::uint64_t second) {
  return hash_internal::MixPair(first, second);
}

template <class... Values>
std::size_t HashOf(const Values&... values) {
  std::size_t seed = 0;
  ((seed = hash_internal::Mix(
        seed, std::hash<std::decay_t<Values>>{}(values))), ...);
  return seed;
}

}  // namespace absl

#endif  // ABSL_HASH_HASH_H_
