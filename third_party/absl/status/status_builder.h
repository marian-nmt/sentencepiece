// Lightweight Abseil StatusBuilder compatibility for standalone SentencePiece.
#ifndef ABSL_STATUS_STATUS_BUILDER_H_
#define ABSL_STATUS_STATUS_BUILDER_H_

#include <sstream>
#include <utility>

#include "third_party/absl/status/status.h"

namespace absl {

class StatusBuilder {
 public:
  explicit StatusBuilder(StatusCode code) : code_(code) {}
  explicit StatusBuilder(Status status)
      : code_(status.code()), initial_message_(status.message()) {}

  template <class Value>
  StatusBuilder& operator<<(const Value& value) {
    stream_ << value;
    return *this;
  }

  operator Status() const {
    std::string message = initial_message_;
    const std::string suffix = stream_.str();
    if (!message.empty() && !suffix.empty()) message += ' ';
    message += suffix;
    return Status(code_, std::move(message));
  }

 private:
  StatusCode code_;
  std::string initial_message_;
  std::ostringstream stream_;
};

}  // namespace absl

#endif  // ABSL_STATUS_STATUS_BUILDER_H_
