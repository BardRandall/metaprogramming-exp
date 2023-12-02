#pragma once

#include <array>
#include <cmath>
#include <limits>
#include <type_traits>
#include <cstdint>
#include <string_view>

namespace detail {
    constexpr std::size_t kLeftShift = 27;
    constexpr std::size_t kRightShift = 1;

    template <auto T>
    constexpr auto helper() {
        return __PRETTY_FUNCTION__;
    }

    template <auto T>
    constexpr auto getTypeString() {
        std::string_view raw_string = helper<T>();
        return raw_string.substr(kLeftShift, raw_string.size() - (kLeftShift + kRightShift));
    }

    template <auto Item>
    constexpr bool IsValidElement() {
        return getTypeString<Item>()[0] != '(';
    }

    template <auto Item>
    constexpr std::string_view getEnumItemName() {
        auto result = getTypeString<Item>();
        std::size_t colon_pos = result.rfind(':');
        if (colon_pos != std::string_view::npos) {
            result.remove_prefix(colon_pos + 1);
        }
        return result;
    };
}  // namespace detail


template <class Enum, std::size_t MAXN = 512>
	requires std::is_enum_v<Enum>
struct EnumeratorTraits {
    static constexpr std::size_t size() noexcept {
        return enum_size;
    }

    static constexpr Enum at(std::size_t i) noexcept {
        return enum_items[i];
    }

    static constexpr std::string_view nameAt(std::size_t i) noexcept {
        return enum_names[i];
    }

    private:
        using UnderlyingType = std::underlying_type_t<Enum>;

        static constexpr long long MINUS_MAXN = -static_cast<long long>(MAXN);
        static constexpr long long MIN_LIMIT = MINUS_MAXN > std::numeric_limits<UnderlyingType>::min() ? MINUS_MAXN : std::numeric_limits<UnderlyingType>::min();
        static constexpr long long MAX_LIMIT = static_cast<long long>(std::min<std::size_t>(MAXN, std::numeric_limits<UnderlyingType>::max()));
        static constexpr std::size_t ITERATION_LIMIT = std::max<std::size_t>(-MIN_LIMIT, MAX_LIMIT);

        static constexpr bool SatisfyLimits(long long index) {
            return (MIN_LIMIT <= index) && (index <= MAX_LIMIT);
        }

        template <long long index>
        static constexpr bool IsValidElement() {
            if (!SatisfyLimits(index)) return false;
            return detail::IsValidElement<static_cast<Enum>(index)>();
        }

        static constexpr std::size_t GetEnumSize() {
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                std::size_t size = 0;
                (
                    [&size]() {
                        constexpr long long index = Is;
                        if (IsValidElement<index>()) ++size;
                        if (Is != 0 && IsValidElement<-index>()) ++size;
                    }(), ...
                );
                return size;
            }(std::make_index_sequence<ITERATION_LIMIT + 1>());
        }

        static constexpr std::size_t enum_size = GetEnumSize();

        static constexpr auto GetEnumItems() {
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                std::size_t start_index = 0;
                std::size_t end_index = enum_size - 1;
                std::array<Enum, enum_size> data;
                (
                    [&start_index, &end_index, &data]() {
                        constexpr long long index = ITERATION_LIMIT - Is;
                        if (IsValidElement<-index>()) {
                            data[start_index++] = static_cast<Enum>(-index);
                        }
                        if (index != 0 && IsValidElement<index>()) {
                            data[end_index--] = static_cast<Enum>(index);
                        }
                    }(), ...
                );
                return data;
            }(std::make_index_sequence<ITERATION_LIMIT + 1>());
        }

        static constexpr auto GetEnumNames() {
            return []<std::size_t... Is>(std::index_sequence<Is...>) {
                std::size_t start_index = 0;
                std::size_t end_index = enum_size - 1;
                std::array<std::string_view, enum_size> data;
                (
                    [&start_index, &end_index, &data]() {
                        constexpr long long index = ITERATION_LIMIT - Is;
                        if (IsValidElement<-index>()) {
                            data[start_index++] = detail::getEnumItemName<static_cast<Enum>(-index)>();
                        }
                        if (index != 0 && IsValidElement<index>()) {
                            data[end_index--] = detail::getEnumItemName<static_cast<Enum>(index)>();
                        }
                    }(), ...
                );
                return data;
            }(std::make_index_sequence<ITERATION_LIMIT + 1>());
        }

        static constexpr auto enum_items = GetEnumItems();
        static constexpr auto enum_names = GetEnumNames();
};
