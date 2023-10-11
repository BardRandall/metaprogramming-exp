#include <span>
#include <concepts>
#include <cstdlib>
#include <array>
#include <iterator>

inline constexpr std::ptrdiff_t dynamic_stride = -1;


namespace detail {
  
  template <typename T, T value, bool store>
  class ValueWrapper;

  template <typename T, T value>
  class ValueWrapper<T, value, true> {
  private:
    T value_;
  public:
    ValueWrapper(T value_) : value_(value_) {}
    T GetValue() const noexcept {
      return value_;
    }
  };
  
  template <typename T, T value>
  class ValueWrapper<T, value, false> {
  public:
    ValueWrapper(T) {}
    constexpr T GetValue() const noexcept {
      return value;
    }
  };
  
  template <std::size_t extent>
  class ExtentHolder : private ValueWrapper<std::size_t, extent, extent == std::dynamic_extent> {
  public:
    ExtentHolder(std::size_t extent_) : ValueWrapper<std::size_t, extent, extent == std::dynamic_extent>(extent_) {
      
    }

    constexpr std::size_t GetExtent() const noexcept {
      return ValueWrapper<std::size_t, extent, extent == std::dynamic_extent>::GetValue();
    }

  };

  template <std::ptrdiff_t stride>
  class StrideHolder : private ValueWrapper<std::ptrdiff_t, stride, stride == dynamic_stride> {
  public:
    StrideHolder(std::ptrdiff_t stride_) : ValueWrapper<std::ptrdiff_t, stride, stride == dynamic_stride>(stride_) {
      
    }

    constexpr std::size_t GetStride() const noexcept {
      return ValueWrapper<std::ptrdiff_t, stride, stride == dynamic_stride>::GetValue();
    }

  };

}  // namespace detail


template
  < class T
  , std::size_t extent = std::dynamic_extent
  , std::ptrdiff_t stride = 1
  >
class Slice : private detail::ExtentHolder<extent>, private detail::StrideHolder<stride> {

  using ExtentHolder = detail::ExtentHolder<extent>;
  using StrideHolder = detail::StrideHolder<stride>;

  template <typename UnderlyingType>
  class IteratorImpl {
  private:
    UnderlyingType* data_;
    std::ptrdiff_t stride_;
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = UnderlyingType;
    using difference_type = std::ptrdiff_t;
    using pointer = UnderlyingType*;
    using reference = UnderlyingType&;

    IteratorImpl() : data_(nullptr), stride_(0) {}
  
    IteratorImpl(UnderlyingType* data, std::ptrdiff_t stride_) : data_(data), stride_(stride_) {}

    IteratorImpl& operator++() noexcept {
      data_ += stride_;
      return *this;
    }

    IteratorImpl operator++(int) noexcept {
      IteratorImpl copy = *this;
      data_ += stride_;
      return copy;
    }

    IteratorImpl& operator--() noexcept {
      data_ -= stride_;
      return *this;
    }

    IteratorImpl operator--(int) noexcept {
      IteratorImpl copy = *this;
      data_ -= stride_;
      return copy;
    }

    IteratorImpl& operator+=(std::ptrdiff_t n) noexcept {
      data_ += n * stride_;
      return *this;
    }

    IteratorImpl& operator-=(std::ptrdiff_t n) noexcept {
      data_ -= n * stride_;
      return *this;
    }

    std::ptrdiff_t operator-(const IteratorImpl& other) const noexcept {
      return (data_ - other.data_) / stride_;
    }

    auto operator<=>(const IteratorImpl& other) const noexcept = default;
    
    UnderlyingType& operator*() const noexcept {
      return *data_;
    }

    UnderlyingType& operator[](std::size_t pos) const noexcept {
      return data_[pos * stride_];
    }

    UnderlyingType* operator->() const noexcept {
      return data_;
    }

  };

public:

  // Typedefs

  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = IteratorImpl<T>;
  using const_iterator = IteratorImpl<const T>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Friends

  template <typename UnderlyingType>
  friend IteratorImpl<UnderlyingType> operator+(const IteratorImpl<UnderlyingType>&, std::ptrdiff_t);

  template <typename UnderlyingType>
  friend typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType> operator+(std::ptrdiff_t n, const typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType>& it);

  template <typename UnderlyingType>
  friend IteratorImpl<UnderlyingType> operator-(const IteratorImpl<UnderlyingType>&, std::ptrdiff_t);

  // Constructors

  Slice() noexcept 
  requires(extent == 0 || extent == std::dynamic_extent) {

  }

  template<class U>
  Slice(U& container) : ExtentHolder(container.size()), StrideHolder(1), data_(std::data(container)) {
    
  }

  template <std::contiguous_iterator It>
  Slice(It first, std::size_t count, std::ptrdiff_t skip = 1) : 
    ExtentHolder(count), StrideHolder(skip), data_(std::to_address(first)) {
  }

  template <std::size_t N>
  Slice(std::array<T, N>& arr) : ExtentHolder(N), StrideHolder(1), data_(std::data(arr)) {

  }

  Slice(const Slice& slice) = default;
  Slice& operator=(const Slice&) = default;
  Slice(Slice&&) = default;
  Slice& operator=(Slice&&) = default;

  ~Slice() = default;

  // Casts

  template <std::size_t other_extent, std::ptrdiff_t other_stride>
  Slice(Slice<T, other_extent, other_stride>& slice) 
  requires (extent == std::dynamic_extent || stride == dynamic_stride)
  : ExtentHolder(slice.Size()), StrideHolder(slice.Stride()), data_(slice.Data()) {

  }

  operator Slice<T, std::dynamic_extent, stride>() const noexcept 
  requires (extent != std::dynamic_extent) {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<T, extent, dynamic_stride>() const noexcept 
  requires (stride != dynamic_stride) {
    return Slice<T, extent, dynamic_stride>(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<T, std::dynamic_extent, dynamic_stride>() const noexcept 
  requires (extent != std::dynamic_extent && stride != dynamic_stride) {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<const T, extent, stride>() const noexcept {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<const T, std::dynamic_extent, stride>() const noexcept
  requires(!std::is_const_v<T> && extent != std::dynamic_extent) {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<const T, extent, dynamic_stride>() const noexcept
  requires(!std::is_const_v<T> && stride != dynamic_stride) {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  operator Slice<const T, std::dynamic_extent, dynamic_stride>() const noexcept 
  requires (!std::is_const_v<T> && extent != std::dynamic_extent && stride != dynamic_stride) {
    return Slice(data_, this->GetExtent(), this->GetStride());
  }

  // Element access

  reference Front() const noexcept {
    return data_[0];
  }

  reference Back() const noexcept {
    return data_[(this->GetExtent() - 1) * this->GetStride()];
  }

  reference operator[](size_type pos) const noexcept {
    return data_[pos * this->GetStride()];
  }

  pointer Data() const noexcept {
    return data_;
  }

  pointer data() const noexcept {  // for STL compatibility
    return Data();
  }

  // Observers

  constexpr size_type Size() const noexcept {
    return this->GetExtent();
  }

  constexpr size_type size() const noexcept {  // for STL compatibility
    return Size();
  }

  constexpr std::ptrdiff_t Stride() const noexcept {
    return this->GetStride();
  }

  [[nodiscard]] constexpr bool empty() const noexcept {
    return this->GetExtent() == 0;
  }

  // Subviews

  auto First(std::size_t count) const noexcept {
    return Slice<T, std::dynamic_extent, stride>(data_, count, this->GetStride());
  }

  template <std::size_t count>
  auto First() const noexcept {
    return Slice<T, count, stride>(data_, count, this->GetStride());
  }

  auto Last(std::size_t count) const noexcept {
    return Slice<T, std::dynamic_extent, stride>(data_ + (this->GetExtent() - count) * this->GetStride(), count, this->GetStride());
  }

  template <std::size_t count>
  auto Last() const noexcept {
    return Slice<T, count, stride>(data_ + (this->GetExtent() - count) * this->GetStride(), count, this->GetStride());
  }

  auto DropFirst(std::size_t count) const noexcept {
    return Slice<T, std::dynamic_extent, stride>(data_ + count * this->GetStride(), this->GetExtent() - count, this->GetStride());
  }

  template <std::size_t count>
  Slice<T, std::dynamic_extent, stride> DropFirst() const noexcept
  requires(extent == std::dynamic_extent) {
    return DropFirst(count);
  }

  template <std::size_t count>
  auto DropFirst() const noexcept
  requires(extent != std::dynamic_extent) {
    return Slice<T, extent - count, stride>(data_ + count * this->GetStride(), extent - count, this->GetStride());
  }

  auto DropLast(std::size_t count) const noexcept {
    return Slice<T, std::dynamic_extent, stride>(data_, this->GetExtent() - count, this->GetStride());
  }

  template <std::size_t count>
  Slice<T, std::dynamic_extent, stride> DropLast() const noexcept
  requires (extent == std::dynamic_extent) {
    return DropLast(count);
  }

  template <std::size_t count>
  auto DropLast() const noexcept
  requires (extent != std::dynamic_extent) {
    return Slice<T, extent - count, stride>(data_, extent - count, this->GetStride());
  }

  // Skips

  auto Skip(std::ptrdiff_t skip) const noexcept {
    return Slice<T, std::dynamic_extent, dynamic_stride>(data_, (this->GetExtent() + skip - 1) / skip, skip * this->GetStride());
  }

  template <std::ptrdiff_t skip>
  auto Skip() const noexcept
  requires(extent == std::dynamic_extent && stride == dynamic_stride) {
    return Slice<T, std::dynamic_extent, dynamic_stride>(data_, (this->GetExtent() + skip - 1) / skip, skip * this->GetStride());
  }

  template <std::ptrdiff_t skip>
  auto Skip() const noexcept
  requires(extent == std::dynamic_extent && stride != dynamic_stride) {
    return Slice<T, std::dynamic_extent, stride * skip>(data_, (this->GetExtent() + skip - 1) / skip, stride * skip);
  }

  template <std::ptrdiff_t skip>
  auto Skip() const noexcept
  requires(extent != std::dynamic_extent && stride == dynamic_stride) {
    return Slice<T, (extent + skip - 1) / skip, dynamic_stride>(data_, (extent + skip - 1) / skip, this->GetStride() * skip);
  }

  template <std::ptrdiff_t skip>
  auto Skip() const noexcept
  requires(extent != std::dynamic_extent && stride != dynamic_stride) {
    return Slice<T, (extent + skip - 1) / skip, skip * stride>(data_, (extent + skip - 1) / skip, skip * stride);
  }

  // Iterator methods

  iterator begin() const noexcept {
    return iterator(data_, this->GetStride());
  }

  const_iterator cbegin() const noexcept {
    return const_iterator(data_, this->GetStride());
  }

  iterator end() const noexcept {
    return iterator(data_ + this->GetExtent() * this->GetStride(), this->GetStride());
  }

  const_iterator cend() const noexcept {
    return const_iterator(data_ + this->GetExtent() * this->GetStride(), this->GetStride());
  }

  reverse_iterator rbegin() const noexcept {
    return std::make_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const noexcept {
    return std::make_reverse_iterator(cbegin());
  }

  reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(end());
  }

  const_reverse_iterator crend() const noexcept {
    return std::make_reverse_iterator(cend());
  }

private:
  T* data_;
  // std::size_t extent_; ?
  // std::ptrdiff_t stride_; ?
};

// Outside of class operators

template <typename T1, typename T2, std::size_t ext1, std::size_t ext2, std::ptrdiff_t str1, std::ptrdiff_t str2>
bool operator==(const Slice<T1, ext1, str1>& s1, const Slice<T2, ext2, str2>& s2) {
  if (s1.Size() != s2.Size()) return false;
  auto it1 = s1.begin();
  auto it2 = s2.begin();
  for (; it1 != s1.end(); ++it1, ++it2) {
    if (*it1 != *it2) return false;
  }
  return true;
}

template <class T, std::size_t extent = std::dynamic_extent, std::ptrdiff_t stride = 1, typename UnderlyingType>
typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType> operator+(const typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType>& it, std::ptrdiff_t n) {
  auto copy = it;
  copy += n;
  return copy;
}

template <class T, std::size_t extent = std::dynamic_extent, std::ptrdiff_t stride = 1, typename UnderlyingType>
typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType> operator+(std::ptrdiff_t n, const typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType>& it) {
  return it + n;
}

template <class T, std::size_t extent = std::dynamic_extent, std::ptrdiff_t stride = 1, typename UnderlyingType>
typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType> operator-(const typename Slice<T, extent, stride>::template IteratorImpl<UnderlyingType>& it, std::ptrdiff_t n) {
  auto copy = it;
  copy -= n;
  return copy;
}

// Deduction guides

template <class Container>
Slice(const Container&) -> Slice<typename Container::value_type, std::dynamic_extent, 1>;

template <class T, std::size_t N>
Slice(std::array<T, N>&) -> Slice<T, N, 1>;

template <class T, std::size_t N>
Slice(const std::array<T, N>&) -> Slice<const T, N, 1>;

template <class It>
Slice(It, std::size_t) -> Slice<std::iter_value_t<It>, std::dynamic_extent, 1>;

template <class It>
Slice(It, std::size_t, std::ptrdiff_t) -> Slice<std::iter_value_t<It>, std::dynamic_extent, dynamic_stride>;