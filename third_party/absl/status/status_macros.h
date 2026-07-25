// Lightweight Abseil status macros compatibility for standalone SentencePiece.
#ifndef ABSL_STATUS_STATUS_MACROS_H_
#define ABSL_STATUS_STATUS_MACROS_H_

#define ABSL_RETURN_IF_ERROR(expression)                                     \
  do {                                                                       \
    const auto status_for_return = (expression);                             \
    if (!status_for_return.ok()) return status_for_return;                   \
  } while (false)

#endif  // ABSL_STATUS_STATUS_MACROS_H_
