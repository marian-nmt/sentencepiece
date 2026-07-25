// C++17 time compatibility for standalone SentencePiece.
#ifndef ABSL_TIME_TIME_H_
#define ABSL_TIME_TIME_H_

#include <chrono>
#include <cstdint>
#include <thread>

namespace absl {

using Time = std::chrono::steady_clock::time_point;
using Duration = std::chrono::steady_clock::duration;

inline Time Now() { return std::chrono::steady_clock::now(); }

inline Duration Milliseconds(int64_t milliseconds) {
  return std::chrono::milliseconds(milliseconds);
}

inline int64_t ToInt64Milliseconds(Duration duration) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

inline void SleepFor(Duration duration) { std::this_thread::sleep_for(duration); }

}  // namespace absl

#endif  // ABSL_TIME_TIME_H_
