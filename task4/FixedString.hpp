#pragma once

#include <algorithm>
#include <array>
#include <string_view>


template<size_t max_length>
struct FixedString {
  constexpr FixedString(const char* string, size_t length) : storage({}), length(length) {
    std::copy(string, string + length, storage.begin());
    // std::fill(storage.begin() + length, storage.end(), '\0');
  }

  constexpr operator std::string_view() const {
    return {storage.data(), length};
  }

  std::array<char, max_length> storage;
  size_t length;
};

constexpr FixedString<256> operator ""_cstr(const char* data, std::size_t length) {
  return FixedString<256>(data, length);
}
