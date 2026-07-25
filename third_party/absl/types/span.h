// Lightweight C++17 absl::Span compatibility for standalone SentencePiece.
#ifndef ABSL_TYPES_SPAN_H_
#define ABSL_TYPES_SPAN_H_

#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace absl {

template <class T>
class Span {
 public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using pointer = T*;
  using reference = T&;
  using iterator = pointer;
  using size_type = std::size_t;

  constexpr Span() = default;
  constexpr Span(pointer data, size_type size) : data_(data), size_(size) {}
  constexpr Span(pointer begin, pointer end)
      : data_(begin), size_(static_cast<size_type>(end - begin)) {}

  template <class Container,
            class Data = decltype(std::declval<Container&>().data()),
            class = std::enable_if_t<std::is_convertible_v<Data, pointer>>>
  constexpr Span(Container& container)
      : data_(container.data()), size_(container.size()) {}

  template <class Container,
            class Data = decltype(std::declval<const Container&>().data()),
            class = std::enable_if_t<std::is_const_v<T> &&
                                     std::is_convertible_v<Data, pointer>>,
            class = void>
  constexpr Span(const Container& container)
      : data_(container.data()), size_(container.size()) {}

    template <class U = T, class = std::enable_if_t<std::is_const_v<U>>>
    Span(std::initializer_list<value_type> values)
      : data_(values.begin()), size_(values.size()) {}

  constexpr iterator begin() const { return data_; }
  constexpr iterator end() const { return data_ + size_; }
  constexpr pointer data() const { return data_; }
  constexpr size_type size() const { return size_; }
  constexpr bool empty() const { return size_ == 0; }
  constexpr reference operator[](size_type index) const { return data_[index]; }
  constexpr reference front() const { return data_[0]; }
  constexpr reference back() const { return data_[size_ - 1]; }

  constexpr Span subspan(size_type offset, size_type count) const {
    return Span(data_ + offset, count);
  }

 private:
  pointer data_ = nullptr;
  size_type size_ = 0;
};

template <class Container>
constexpr auto MakeConstSpan(const Container& container)
    -> Span<const typename Container::value_type> {
  return {container.data(), container.size()};
}

template <class T, std::size_t N>
constexpr Span<const T> MakeConstSpan(const T (&array)[N]) {
  return {array, N};
}

}  // namespace absl

#endif  // ABSL_TYPES_SPAN_H_
