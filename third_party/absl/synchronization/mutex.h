// C++17 mutex compatibility for standalone SentencePiece.
#ifndef ABSL_SYNCHRONIZATION_MUTEX_H_
#define ABSL_SYNCHRONIZATION_MUTEX_H_

#include <condition_variable>
#include <functional>
#include <mutex>

namespace absl {

class Condition {
 public:
  template <class Object>
  Condition(Object* object, bool (Object::*method)() const)
      : predicate_([object, method] { return (object->*method)(); }) {}

  bool Eval() const { return predicate_(); }

 private:
  std::function<bool()> predicate_;
};

class Mutex {
 public:
  void Lock() { mutex_.lock(); }

  void Unlock() {
    mutex_.unlock();
    condition_.notify_all();
  }

  void Await(const Condition& condition) {
    std::unique_lock<std::mutex> lock(mutex_, std::adopt_lock);
    condition_.wait(lock, [&condition] { return condition.Eval(); });
    lock.release();
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
};

class MutexLock {
 public:
  explicit MutexLock(Mutex* mutex) : mutex_(mutex) { mutex_->Lock(); }
  explicit MutexLock(Mutex& mutex) : MutexLock(&mutex) {}
  ~MutexLock() { mutex_->Unlock(); }

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;

 private:
  Mutex* mutex_;
};

}  // namespace absl

#endif  // ABSL_SYNCHRONIZATION_MUTEX_H_
