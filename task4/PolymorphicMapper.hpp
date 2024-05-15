#pragma once

#include <exception>
#include <optional>
#include <type_traits>
#include <typeinfo>


namespace detail {
  class break_exception : std::exception {

  };
}  // namespace detail


template <class From, auto target>
struct Mapping {

};

template <class Base, class Target, class... Mappings>
struct PolymorphicMapper {
  static std::optional<Target> map(const Base& object) {
    std::optional<Target> result;
    try {
      (
        [&object, &result]<class From, Target target>(Mapping<From, target>) {
          try {
            [[maybe_unused]] auto& unused =  dynamic_cast<const From&>(object);
            result = target;
            throw detail::break_exception();
          } catch(std::bad_cast&) {
            
          }
        }(Mappings{})
        , ...
      );
    } catch (detail::break_exception&) {
      return result;
    }
    return std::nullopt;
  }
};
