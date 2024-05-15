#pragma once

#include <utility>
#include <type_lists.hpp>
#include <value_types.hpp>

// Nats
namespace detail {
    using namespace value_types;

    template <VTag<int> tag>
    using PlusOne = ValueTag<tag::Value + 1>;
}

using Nats = type_lists::Iterate<detail::PlusOne, value_types::ValueTag<0>>;

// Fib
namespace detail {
    template <VTag<std::pair<int, int>> tag>
    using FibNext = ValueTag<std::make_pair(tag::Value.first + tag::Value.second, tag::Value.first)>;

    template <VTag<std::pair<int, int>> tag>
    using DropSecond = ValueTag<tag::Value.first>;
}

using Fib = type_lists::Map
            < detail::DropSecond
            , type_lists::Iterate
                < detail::FibNext
                , value_types::ValueTag<std::make_pair(0, 1)>
                >
            >;

// Primes
namespace detail {
    template <int N, int D>
    struct IsPrimeHelper {
        static constexpr bool Value = (N % D == 0) ? false : IsPrimeHelper<N, D - 1>::Value;
    };

    template <int N>
    struct IsPrimeHelper<N, 1> {
        static constexpr bool Value = true;
    };

    template <VTag<int> tag>
    struct IsPrime {
        static constexpr bool Value = IsPrimeHelper<tag::Value, tag::Value - 1>::Value;
    };

    template <>
    struct IsPrime<ValueTag<0>> {
        static constexpr bool Value = false;
    };

    template <>
    struct IsPrime<ValueTag<1>> {
        static constexpr bool Value = false;
    };
}

using Primes = type_lists::Filter
               < detail::IsPrime
               , Nats
               >;
