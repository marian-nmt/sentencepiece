// C++17 scope cleanup compatibility for standalone SentencePiece.
#ifndef ABSL_CLEANUP_CLEANUP_H_
#define ABSL_CLEANUP_CLEANUP_H_

#include <type_traits>
#include <utility>

namespace absl {

template <class Callback>
class Cleanup {
 public:
  Cleanup(Callback callback) : callback_(std::move(callback)) {}
  Cleanup(Cleanup&& other) noexcept
      : callback_(std::move(other.callback_)), active_(other.active_) {
    other.active_ = false;
  }
  Cleanup(const Cleanup&) = delete;
  Cleanup& operator=(const Cleanup&) = delete;
  ~Cleanup() {
    if (active_) callback_();
  }

 private:
  Callback callback_;
  bool active_ = true;
};

template <class Callback>
Cleanup(Callback) -> Cleanup<std::decay_t<Callback>>;

}  // namespace absl

#endif  // ABSL_CLEANUP_CLEANUP_H_
