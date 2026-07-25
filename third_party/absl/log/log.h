// Lightweight Abseil logging compatibility for standalone SentencePiece.
#ifndef ABSL_LOG_LOG_H_
#define ABSL_LOG_LOG_H_

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace absl {

enum class LogSeverityAtLeast : int {
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
  kINFO = kInfo,
  kWARNING = kWarning,
  kERROR = kError,
  kFATAL = kFatal,
};

namespace log_internal {

inline std::atomic<int>& MinLogLevelStorage() {
  static std::atomic<int> level{0};
  return level;
}

inline const char* BaseName(const char* path) {
  const char* base = path;
  for (const char* cursor = path; *cursor != '\0'; ++cursor) {
    if (*cursor == '/' || *cursor == '\\') base = cursor + 1;
  }
  return base;
}

class LogMessage {
 public:
  LogMessage(LogSeverityAtLeast severity, const char* file, int line)
      : severity_(severity), file_(file), line_(line) {}

  ~LogMessage() {
    if (static_cast<int>(severity_) >= MinLogLevelStorage().load()) {
      std::cerr << BaseName(file_) << '(' << line_ << ") " << stream_.str()
                << std::endl;
    }
    if (severity_ == LogSeverityAtLeast::kFatal) std::abort();
  }

  std::ostream& stream() { return stream_; }

 private:
  LogSeverityAtLeast severity_;
  const char* file_;
  int line_;
  std::ostringstream stream_;
};

}  // namespace log_internal

inline void SetMinLogLevel(LogSeverityAtLeast severity) {
  log_internal::MinLogLevelStorage().store(static_cast<int>(severity));
}

inline void SetStderrThreshold(LogSeverityAtLeast) {}

}  // namespace absl

#define LOG(severity)                                                        \
  ::absl::log_internal::LogMessage(                                          \
      ::absl::LogSeverityAtLeast::k##severity, __FILE__, __LINE__)           \
      .stream()

#endif  // ABSL_LOG_LOG_H_
