#include <bits/iterator_concepts.h>
#include <bits/ranges_base.h>
#include <span>
#include <concepts>
#include <cstdlib>
#include <iterator>
#include <cassert>


namespace detail {

  template <std::size_t extent>
  class ExtentHolder {
  public:
    [[gnu::always_inline]] ExtentHolder(std::size_t size) { assert(extent <= size); }

    constexpr std::size_t GetExtent() const noexcept {
      return extent;
    }
  };

  template <>
  class ExtentHolder<std::dynamic_extent> {
    std::size_t extent_;
  public:
    [[gnu::always_inline]] ExtentHolder(std::size_t size) : extent_(size) {

    }

    [[gnu::always_inline]] std::size_t GetExtent() const noexcept {
      return extent_;
    }

  };

}  // namespace detail


template <typename T, std::size_t extent = std::dynamic_extent>
class Span : private detail::ExtentHolder<extent> {
private:
  using ExtentHolder = detail::ExtentHolder<extent>;
public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;
  using const_iterator = const iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  // Reimplement the standard span interface here
  // (some more exotic methods are not checked by the tests and can be sipped)
  // Note that unliike std, the methods name should be Capitalized!
  // E.g. instead of subspan, do Subspan.
  // Note that this does not apply to iterator methods like begin/end/etc.

  // Constructors

  Span() requires (extent == 0 || extent == std::dynamic_extent) 
    : ExtentHolder(0), data_(nullptr) {
    
  }

  template <std::contiguous_iterator It>
  Span(It first, size_type count) : ExtentHolder(count), data_(std::to_address(first)) {
    
  }

  template <std::contiguous_iterator It>
  Span(It first, It last) : ExtentHolder(last - first), data_(std::to_address(first)) {

  }

  template <std::ranges::contiguous_range Range>
  explicit(extent != std::dynamic_extent)
  Span(Range&& range) : ExtentHolder(std::ranges::size(range)), data_(&*std::ranges::begin(range)) {

  }

  template <class U, std::size_t N>
  Span(const std::span<U, N>& span) : ExtentHolder(span.Size()), data_(span.Data()) {
    
  }

  Span(const Span&) = default;

  Span& operator=(const Span&) = default;

  // Element access

  reference Front() const {
    return *begin();
  }

  reference Back() const {
    return *(end() - 1);
  }

  reference operator[](size_type index) const {
    return data_[index];
  }

  pointer Data() const {
    return data_;
  }

  // Observers

  constexpr size_type Size() const {
    return ExtentHolder::GetExtent();
  }

  constexpr size_type SizeBytes() const {
    return Size() * sizeof(T);
  }

  [[nodiscard]] constexpr bool empty() const {
    return Size() == 0;
  }

  // Subviews

  template <std::size_t Count>
  Span<T, Count> First() const {
    return Span<T, Count>(Data(), Count);
  }

  Span<T, std::dynamic_extent> First(std::size_t Count) const {
    return Span<T, std::dynamic_extent>(Data(), Count);
  }

  template <std::size_t Count>
  Span<T, Count> Last() const {
    return Span<T, Count>(Data() + Size() - Count, Count);
  }

  Span<T, std::dynamic_extent> Last(std::size_t Count) const {
    return Span<T, std::dynamic_extent>(Data() + Size() - Count, Count);
  }

  // Iterator methods

  iterator begin() const {
    return data_;
  }

  const_iterator cbegin() const {
    return begin();
  }

  reverse_iterator rbegin() const {
    std::make_reverse_iterator(end());
  }

  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cend());
  }

  iterator end() const {
    return data_ + Size();
  }

  const_iterator cend() const {
    return end();
  }

  reverse_iterator rend() const {
    std::make_reverse_iterator(begin());
  }

  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cbegin());
  }


private:
  T* data_ = nullptr;
};


template <class It, class EndOrSize>
Span( It, EndOrSize ) -> Span<std::remove_reference_t<std::iter_reference_t<It>>>;

template< class T, std::size_t N >
Span( T (&)[N] ) -> Span<T, N>;

template< class T, std::size_t N >
Span( std::array<T, N>& ) -> Span<T, N>;

template< class T, std::size_t N >
Span( const std::array<T, N>& ) -> Span<const T, N>;

template <class C>
Span(const C&) -> Span<typename C::value_type, std::dynamic_extent>;