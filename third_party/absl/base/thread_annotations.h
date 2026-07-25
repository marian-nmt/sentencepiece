// No-op thread annotations for standalone SentencePiece.
#ifndef ABSL_BASE_THREAD_ANNOTATIONS_H_
#define ABSL_BASE_THREAD_ANNOTATIONS_H_

#define ABSL_GUARDED_BY(mutex)
#define ABSL_PT_GUARDED_BY(mutex)
#define ABSL_EXCLUSIVE_LOCKS_REQUIRED(...)
#define ABSL_SHARED_LOCKS_REQUIRED(...)
#define ABSL_LOCKS_EXCLUDED(...)
#define ABSL_NO_THREAD_SAFETY_ANALYSIS

#endif  // ABSL_BASE_THREAD_ANNOTATIONS_H_
