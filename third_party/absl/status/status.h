// Lightweight Abseil Status compatibility for standalone SentencePiece.
#ifndef ABSL_STATUS_STATUS_H_
#define ABSL_STATUS_STATUS_H_

#include "src/sentencepiece_status.h"

namespace absl {

using sentencepiece::AbortedError;
using sentencepiece::AlreadyExistsError;
using sentencepiece::CancelledError;
using sentencepiece::DataLossError;
using sentencepiece::DeadlineExceededError;
using sentencepiece::FailedPreconditionError;
using sentencepiece::InternalError;
using sentencepiece::InvalidArgumentError;
using sentencepiece::IsNotFound;
using sentencepiece::NotFoundError;
using sentencepiece::OkStatus;
using sentencepiece::OutOfRangeError;
using sentencepiece::PermissionDeniedError;
using sentencepiece::ResourceExhaustedError;
using sentencepiece::Status;
using sentencepiece::StatusCode;
using sentencepiece::UnauthenticatedError;
using sentencepiece::UnavailableError;
using sentencepiece::UnimplementedError;
using sentencepiece::UnknownError;

}  // namespace absl

#endif  // ABSL_STATUS_STATUS_H_
