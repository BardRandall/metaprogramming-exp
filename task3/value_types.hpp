#pragma once
#include "type_tuples.hpp"

namespace value_types
{

template<auto V>
struct ValueTag{ static constexpr auto Value = V; };

template<class T, T... ts>
using VTuple = type_tuples::TTuple<ValueTag<ts>...>;

template<class Tag, class T>
concept VTag = requires(Tag t) { []<T x>(ValueTag<x>){}(t); };

}
