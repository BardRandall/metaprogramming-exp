#include <span>
#include <concepts>
#include <cstdlib>
#include <iterator>


template
  < class T
  , std::size_t extent = std::dynamic_extent
  >
class Span {
private:
  template <std::size_t extent_>
  class SpanStorage {
      T* data_ = nullptr;
    public:
      SpanStorage(T* data, std::size_t) : data_(data) {

      }

      constexpr std::size_t Size() const {
        return extent_;
      }

      T* Data() const {
        return data_;
      }
  };

  template <>
  class SpanStorage<std::dynamic_extent> {
      T* data_ = nullptr;
      std::size_t size_ = 0;
    public:
      SpanStorage(T* data, std::size_t size) : data_(data), size_(size) {

      }

      constexpr std::size_t Size() const {
        return size_;
      }

      T* Data() const {
        return data_;
      }
  };

public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;  // TODO?
  using const_iterator = const iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  // Reimplement the standard span interface here
  // (some more exotic methods are not checked by the tests and can be sipped)
  // Note that unliike std, the methods name should be Capitalized!
  // E.g. instead of subspan, do Subspan.
  // Note that this does not apply to iterator methods like begin/end/etc.

  // Constructors

  Span() 
  requires (extent == 0 || extent == std::dynamic_extent) {
    
  }

  template <class It>
  Span(It first, size_type count) : storage_(std::to_address(first), count) {
    
  }

  template <class It, class End>
  Span(It first, End last) : storage_(std::to_address(first), last - first) {

  }

  // template <std::size_t N>
  // requires (extent == N || extent == std::dynamic_extent)
  // Span(std::type_identity_t<element_type> (&arr)[N]) : storage(std::data(arr), N) {

  // }

  // template <class U, std::size_t N>
  // Span(std::array<U, N>& arr) : storage(std::data(arr), N) {

  // }

  // template <class U, std::size_t N>
  // Span(const std::array<U, N>& arr) : storage(std::data(arr), N) {
    
  // }

  template <class C>
  requires requires (C c) {c.begin(); c.end();}
  Span(C& container) : Span(container.begin(), container.end()) {
    
  }

  template <class U, std::size_t N>
  Span(const std::span<U, N>& source) : storage_(source.Data(), source.Size()) {
    
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
    return storage_.Data()[index];
  }

  pointer Data() const {
    return storage_.Data();
  }

  // Observers

  constexpr size_type Size() const {
    return storage_.Size();
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

  // template <std::size_t Offset, std::size_t Count = std::dynamic_extent >
  // std::span<T, E /* see below */> Subspan() const {

  // }

  // template <std::size_t Offset>
  // std::span<T, std::dynamic_extent> Subspan(size_type Offset, size_type Count = std::dynamic_extent) const {

  // }

  // Iterator methods

  iterator begin() const {
    return storage_.Data();
  }

  const_iterator cbegin() const {
    return begin();
  }

  reverse_iterator rbegin() const {
    std::make_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const {
    return std::make_reverse_iterator(cbegin());
  }

  iterator end() const {
    return storage_.Data() + storage_.Size();
  }

  const_iterator cend() const {
    return end();
  }

  reverse_iterator rend() const {
    std::make_reverse_iterator(end());
  }

  const_reverse_iterator crend() const {
    return std::make_reverse_iterator(cend());
  }


private:
  SpanStorage<extent> storage_;
  // T* data_ = nullptr;
  // std::size_t extent_; ?
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