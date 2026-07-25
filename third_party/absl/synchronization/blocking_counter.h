// C++17 blocking counter compatibility for standalone SentencePiece.
#ifndef ABSL_SYNCHRONIZATION_BLOCKING_COUNTER_H_
#define ABSL_SYNCHRONIZATION_BLOCKING_COUNTER_H_

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace absl {

class BlockingCounter {
 public:
  explicit BlockingCounter(std::size_t count) : count_(count) {}

  void DecrementCount() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ > 0 && --count_ == 0) condition_.notify_all();
  }

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this] { return count_ == 0; });
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  std::size_t count_;
};

}  // namespace absl

#endif  // ABSL_SYNCHRONIZATION_BLOCKING_COUNTER_H_
