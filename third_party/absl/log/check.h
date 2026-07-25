// Lightweight Abseil CHECK compatibility for standalone SentencePiece.
#ifndef ABSL_LOG_CHECK_H_
#define ABSL_LOG_CHECK_H_

#include "third_party/absl/log/log.h"

#define ABSL_INTERNAL_CHECK(condition, text)                                 \
  switch (0)                                                                 \
  case 0:                                                                    \
  default:                                                                   \
    if (condition) {                                                         \
    } else                                                                   \
      ::absl::log_internal::LogMessage(                                      \
          ::absl::LogSeverityAtLeast::kFatal, __FILE__, __LINE__)            \
              .stream()                                                      \
          << "Check failed: " text " "

#define CHECK(condition) ABSL_INTERNAL_CHECK((condition), #condition)
#define QCHECK(condition) CHECK(condition)
#define DCHECK(condition) CHECK(condition)

#define CHECK_EQ(a, b) CHECK((a) == (b))
#define CHECK_NE(a, b) CHECK((a) != (b))
#define CHECK_LE(a, b) CHECK((a) <= (b))
#define CHECK_LT(a, b) CHECK((a) < (b))
#define CHECK_GE(a, b) CHECK((a) >= (b))
#define CHECK_GT(a, b) CHECK((a) > (b))

#define QCHECK_EQ(a, b) CHECK_EQ(a, b)
#define QCHECK_NE(a, b) CHECK_NE(a, b)
#define QCHECK_LE(a, b) CHECK_LE(a, b)
#define QCHECK_LT(a, b) CHECK_LT(a, b)
#define QCHECK_GE(a, b) CHECK_GE(a, b)
#define QCHECK_GT(a, b) CHECK_GT(a, b)

#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)

#define CHECK_OK(expression)                                                 \
  do {                                                                       \
    const auto& status_for_check = (expression);                             \
    CHECK(status_for_check.ok()) << status_for_check.ToString();             \
  } while (false)
#define QCHECK_OK(expression) CHECK_OK(expression)
#define DCHECK_OK(expression) CHECK_OK(expression)

#endif  // ABSL_LOG_CHECK_H_
