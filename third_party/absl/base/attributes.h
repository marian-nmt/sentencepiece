// Lightweight Abseil attributes compatibility for standalone SentencePiece.
#ifndef ABSL_BASE_ATTRIBUTES_H_
#define ABSL_BASE_ATTRIBUTES_H_

#define ABSL_MUST_USE_RESULT [[nodiscard]]
#define ABSL_ATTRIBUTE_NOINLINE __attribute__((noinline))
#define ABSL_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define ABSL_ATTRIBUTE_COLD __attribute__((cold))
#define ABSL_ATTRIBUTE_UNUSED [[maybe_unused]]
#define ABSL_DEPRECATED(message) [[deprecated(message)]]

#endif  // ABSL_BASE_ATTRIBUTES_H_
