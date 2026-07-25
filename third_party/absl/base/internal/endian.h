// Lightweight endian compatibility for standalone SentencePiece.
#ifndef ABSL_BASE_INTERNAL_ENDIAN_H_
#define ABSL_BASE_INTERNAL_ENDIAN_H_

#include <cstdint>
#include <cstring>

namespace absl {

enum class endian {
  little,
  big,
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  native = big,
#else
  native = little,
#endif
};

inline uint32_t gbswap_32(uint32_t value) {
#if defined(_MSC_VER)
  return _byteswap_ulong(value);
#else
  return __builtin_bswap32(value);
#endif
}

namespace little_endian {

inline uint32_t Load32(const void* source) {
  uint32_t value;
  std::memcpy(&value, source, sizeof(value));
  if (endian::native == endian::big) value = gbswap_32(value);
  return value;
}

inline void Store32(void* destination, uint32_t value) {
  if (endian::native == endian::big) value = gbswap_32(value);
  std::memcpy(destination, &value, sizeof(value));
}

}  // namespace little_endian
}  // namespace absl

#endif  // ABSL_BASE_INTERNAL_ENDIAN_H_
