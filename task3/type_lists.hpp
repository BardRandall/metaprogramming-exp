#pragma once

#include <concepts>

#include <csignal>
#include <type_traits>
#include <type_tuples.hpp>


namespace type_lists
{

template<class TL>
concept TypeSequence =
    requires {
        typename TL::Head;
        typename TL::Tail;
    };

struct Nil {};

template<class TL>
concept Empty = std::derived_from<TL, Nil>;

template<class TL>
concept TypeList = Empty<TL> || TypeSequence<TL>;

template <typename T, typename TL>
struct Cons {
    using Head = T;
    using Tail = TL;
};

// FromTuple
namespace detail {
    template <type_tuples::TypeTuple TT>
    struct FromTupleImpl;

    template <typename Head, typename... Tail>
    struct FromTupleImpl<type_tuples::TTuple<Head, Tail...>> {
        using Type = Cons<Head, typename FromTupleImpl<type_tuples::TTuple<Tail...>>::Type>;
    };

    template <>
    struct FromTupleImpl<type_tuples::TTuple<>> {
        using Type = Nil;
    };
}

template <type_tuples::TypeTuple TT>
using FromTuple = typename detail::FromTupleImpl<TT>::Type;

// ToTuple
namespace detail {
    template <TypeList TL, typename... Accumulated>
    struct ToTupleImpl {
        using Type = typename ToTupleImpl<typename TL::Tail, Accumulated..., typename TL::Head>::Type;
    };

    template <Empty TL, typename... Accumulated>
    struct ToTupleImpl<TL, Accumulated...> {
        using Type = type_tuples::TTuple<Accumulated...>;
    };
}

template <TypeList TL>
using ToTuple = typename detail::ToTupleImpl<TL>::Type;

// Repeat
template <typename T>
struct Repeat {
    using Head = T;
    using Tail = Repeat<T>;
};

// Take
template <std::size_t N, TypeList TL>
struct Take {
    using Head = typename TL::Head;
    using Tail = Take<N - 1, typename TL::Tail>;
};

template <TypeList TL>
struct Take<0, TL> : Nil {

};

template <std::size_t N, Empty TL>
struct Take<N, TL> : Nil {

};

// Drop
namespace detail {
    template <std::size_t N, TypeList TL>
    struct DropImpl {
        using Type = typename DropImpl<N - 1, typename TL::Tail>::Type;
    };

    template <TypeList TL>
    struct DropImpl<0, TL> {
        using Type = TL;
    };

    template <std::size_t N, Empty TL>
    struct DropImpl<N, TL> {
        using Type = TL;
    };
}

template <std::size_t N, TypeList TL>
using Drop = typename detail::DropImpl<N, TL>::Type;

// Replicate
namespace detail {
    template <std::size_t N, typename T>
    struct ReplicateImpl {
        using Type = Cons<T, typename ReplicateImpl<N - 1, T>::Type>;
    };

    template <typename T>
    struct ReplicateImpl<0, T> {
        using Type = Nil;
    };
}

template <std::size_t N, typename T>
using Replicate = typename detail::ReplicateImpl<N, T>::Type;

// Map
template <template <typename> class F, TypeList TL>
struct Map {
    using Head = F<typename TL::Head>;
    using Tail = Map<F, typename TL::Tail>;
};

template <template <typename> class F, Empty TL>
struct Map<F, TL> : Nil {
    
};

// Filter
namespace detail {

    template <template <typename> class P, TypeList TL, bool = false>
    struct FilterImpl;

    template <template <typename> class P, TypeList TL>
    struct FilterDispatcherImpl;

    template <template <typename> class P, TypeSequence TL>
    struct FilterDispatcherImpl<P, TL> {
        using Type = FilterImpl<P, TL, P<typename TL::Head>::Value>;
    };

    template <template <typename> class P, Empty TL>
    struct FilterDispatcherImpl<P, TL> {
        using Type = FilterImpl<P, TL>;
    };

    template <template <typename> class P, TypeList TL>
    using FilterDispatcher = typename FilterDispatcherImpl<P, TL>::Type;

    template <template <typename> class P, Empty TL>
    struct FilterImpl<P, TL> : Nil {
        // using Head = Nil;  // for compatibility
        // using Tail = Nil;  // for compatibility
    };

    template <template <typename> class P, TypeSequence TL>
    struct FilterImpl<P, TL, true> {
        using Head = typename TL::Head;
        using Tail = FilterDispatcher<P, typename TL::Tail>;
    };

    template <template <typename> class P, TypeSequence TL>
    struct FilterImpl<P, TL, false> : FilterDispatcher<P, typename TL::Tail> {
        using typename FilterDispatcher<P, typename TL::Tail>::Head;
        using typename FilterDispatcher<P, typename TL::Tail>::Tail;
    };

}

template <template <typename> class P, TypeList TL>
using Filter = detail::FilterDispatcher<P, TL>;

// Iterate
namespace detail {
    template <template <typename> class F, typename T>
    struct IterateImpl {
        using Head = T;
        using Tail = IterateImpl<F, F<T>>;
    };
}

template <template <typename> class F, typename T>
using Iterate = detail::IterateImpl<F, T>;

// Cycle
namespace detail {
    template <TypeList TL, typename... Ts>
    struct CycleImpl {
        using Head = typename TL::Head;
        using Tail = CycleImpl<typename TL::Tail, Ts..., typename TL::Head>;
    };

    template <Empty TL, typename T, typename... Ts>
    struct CycleImpl<TL, T, Ts...> {
        using Head = T;
        using Tail = CycleImpl<TL, Ts..., T>;
    };

    template <Empty TL>
    struct CycleImpl<TL> : Nil {

    };
}

template <TypeList TL>
using Cycle = detail::CycleImpl<TL>;
// because Cycle should have just one template argument

// Inits
template <TypeList TL, typename... Ts>
struct Inits {
    using Head = FromTuple<type_tuples::TTuple<Ts...>>;
    using Tail = Inits<typename TL::Tail, Ts..., typename TL::Head>;
};

template <Empty TL, typename... Ts>
struct Inits<TL, Ts...> {
    using Head = FromTuple<type_tuples::TTuple<Ts...>>;
    using Tail = Nil;
};

// Tails
template <TypeList TL>
struct Tails {
    using Head = TL;
    using Tail = Tails<typename TL::Tail>;
};

template <Empty TL>
struct Tails<TL> {
    using Head = Nil;
    using Tail = Nil;
};

// Scanl
namespace detail {
    template <template <typename, typename> class OP, typename T, TypeList TL>
    struct ScanlImpl {
        using Type = Cons<T, typename ScanlImpl<OP, OP<T, typename TL::Head>, typename TL::Tail>::Type>;
    };

    template <template <typename, typename> class OP, typename T, Empty TL>
    struct ScanlImpl<OP, T, TL> {
        using Type = Cons<T, Nil>;
    };

}

template <template <typename, typename> class OP, typename T, TypeList TL>
using Scanl = typename detail::ScanlImpl<OP, T, TL>::Type;

// Foldl
namespace detail {
    template <template <typename, typename> class OP, typename T, TypeList TL>
    struct FoldlImpl {
        using Type = OP<typename FoldlImpl<OP, T, typename TL::Tail>::Type, typename TL::Head>;
    };

    template <template <typename, typename> class OP, typename T, Empty TL>
    struct FoldlImpl<OP, T, TL> {
        using Type = T;
    };
}

template <template <typename, typename> class OP, typename T, TypeList TL>
using Foldl = typename detail::FoldlImpl<OP, T, TL>::Type;

// Zip2
template <TypeList L, TypeList R>
struct Zip2 {
    using Head = type_tuples::TTuple<typename L::Head, typename R::Head>;
    using Tail = Zip2<typename L::Tail, typename R::Tail>;
};

template <Empty L, TypeList R>
struct Zip2<L, R> : Nil {};

template <TypeList L, Empty R>
struct Zip2<L, R> : Nil {};

template <Empty L, Empty R>
struct Zip2<L, R> : Nil {};

// Zip
// TODO for finite lists
template <TypeList... TL>
struct Zip {
    using Head = type_tuples::TTuple<typename TL::Head...>;
    using Tail = Zip<typename TL::Tail...>;
};

} // namespace type_lists
