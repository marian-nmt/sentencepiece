#ifndef SENTENCEPIECE_STATUS_H_
#define SENTENCEPIECE_STATUS_H_

#include <ostream>
#include <string>
#include <utility>

namespace sentencepiece {

enum class StatusCode : int {
  kOk = 0,
  kCancelled = 1,
  kUnknown = 2,
  kInvalidArgument = 3,
  kDeadlineExceeded = 4,
  kNotFound = 5,
  kAlreadyExists = 6,
  kPermissionDenied = 7,
  kResourceExhausted = 8,
  kFailedPrecondition = 9,
  kAborted = 10,
  kOutOfRange = 11,
  kUnimplemented = 12,
  kInternal = 13,
  kUnavailable = 14,
  kDataLoss = 15,
  kUnauthenticated = 16,
};

class Status {
 public:
  Status() = default;
  Status(StatusCode code, std::string message)
      : code_(code), message_(std::move(message)) {
    if (code_ == StatusCode::kOk) message_.clear();
  }

  [[nodiscard]] bool ok() const { return code_ == StatusCode::kOk; }
  [[nodiscard]] StatusCode code() const { return code_; }
  [[nodiscard]] const std::string& message() const { return message_; }

  [[nodiscard]] std::string ToString() const {
    if (ok()) return "OK";
    return CodeName(code_) + ": " + message_;
  }

  void IgnoreError() const {}

  void Update(const Status& status) {
    if (ok() && !status.ok()) *this = status;
  }

  friend bool operator==(const Status& lhs, const Status& rhs) {
    return lhs.code_ == rhs.code_ && lhs.message_ == rhs.message_;
  }

  friend bool operator!=(const Status& lhs, const Status& rhs) {
    return !(lhs == rhs);
  }

 private:
  static std::string CodeName(StatusCode code) {
    switch (code) {
      case StatusCode::kOk: return "OK";
      case StatusCode::kCancelled: return "CANCELLED";
      case StatusCode::kUnknown: return "UNKNOWN";
      case StatusCode::kInvalidArgument: return "INVALID_ARGUMENT";
      case StatusCode::kDeadlineExceeded: return "DEADLINE_EXCEEDED";
      case StatusCode::kNotFound: return "NOT_FOUND";
      case StatusCode::kAlreadyExists: return "ALREADY_EXISTS";
      case StatusCode::kPermissionDenied: return "PERMISSION_DENIED";
      case StatusCode::kResourceExhausted: return "RESOURCE_EXHAUSTED";
      case StatusCode::kFailedPrecondition: return "FAILED_PRECONDITION";
      case StatusCode::kAborted: return "ABORTED";
      case StatusCode::kOutOfRange: return "OUT_OF_RANGE";
      case StatusCode::kUnimplemented: return "UNIMPLEMENTED";
      case StatusCode::kInternal: return "INTERNAL";
      case StatusCode::kUnavailable: return "UNAVAILABLE";
      case StatusCode::kDataLoss: return "DATA_LOSS";
      case StatusCode::kUnauthenticated: return "UNAUTHENTICATED";
    }
    return "UNKNOWN";
  }

  StatusCode code_ = StatusCode::kOk;
  std::string message_;
};

inline std::ostream& operator<<(std::ostream& stream, const Status& status) {
  return stream << status.ToString();
}

inline Status OkStatus() { return {}; }

#define SPM_STATUS_FACTORY(name, code)                   \
  inline Status name(std::string message) {              \
    return Status(StatusCode::code, std::move(message)); \
  }

SPM_STATUS_FACTORY(CancelledError, kCancelled)
SPM_STATUS_FACTORY(UnknownError, kUnknown)
SPM_STATUS_FACTORY(InvalidArgumentError, kInvalidArgument)
SPM_STATUS_FACTORY(DeadlineExceededError, kDeadlineExceeded)
SPM_STATUS_FACTORY(NotFoundError, kNotFound)
SPM_STATUS_FACTORY(AlreadyExistsError, kAlreadyExists)
SPM_STATUS_FACTORY(PermissionDeniedError, kPermissionDenied)
SPM_STATUS_FACTORY(ResourceExhaustedError, kResourceExhausted)
SPM_STATUS_FACTORY(FailedPreconditionError, kFailedPrecondition)
SPM_STATUS_FACTORY(AbortedError, kAborted)
SPM_STATUS_FACTORY(OutOfRangeError, kOutOfRange)
SPM_STATUS_FACTORY(UnimplementedError, kUnimplemented)
SPM_STATUS_FACTORY(InternalError, kInternal)
SPM_STATUS_FACTORY(UnavailableError, kUnavailable)
SPM_STATUS_FACTORY(DataLossError, kDataLoss)
SPM_STATUS_FACTORY(UnauthenticatedError, kUnauthenticated)

#undef SPM_STATUS_FACTORY

inline bool IsNotFound(const Status& status) {
  return status.code() == StatusCode::kNotFound;
}

}  // namespace sentencepiece

#endif  // SENTENCEPIECE_STATUS_H_
