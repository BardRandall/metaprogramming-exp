#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "generated.hpp"

template <class...>
class Annotate {};

namespace detail {
    template <class Instance, template <class...> class MetaFunc>
    concept IsSpec = requires (Instance inst) {
        []<typename... Args>(MetaFunc<Args...>) {

        }(inst);
    };

    template <class T, template <class...> class MetaFunc>
    struct is_specialization : std::false_type {};

    template <template <class...> class MetaFunc, class... Args>
    struct is_specialization<MetaFunc<Args...>, MetaFunc> : std::true_type {
        using Type = MetaFunc<Args...>;
    };

    struct Idle {};

    template <template <class...> class AnnotationTemplate, class... Annos>
    struct FindAnnotationImpl;

    template <template <class...> class AnnotationTemplate, class Anno, class... Annos>
    requires (is_specialization<Anno, AnnotationTemplate>::value)
    struct FindAnnotationImpl<AnnotationTemplate, Anno, Annos...> {
        using Type = Anno;
    };

    template <template <class...> class AnnotationTemplate, class Anno, class... Annos>
    requires (!is_specialization<Anno, AnnotationTemplate>::value)
    struct FindAnnotationImpl<AnnotationTemplate, Anno, Annos...> {
        using Type = FindAnnotationImpl<AnnotationTemplate, Annos...>::Type;
    };
}

template <class T, std::size_t I, class Field, class... Annos>
struct FieldDescriptor {
    using Type = Field;
    using Annotations = Annotate<Annos...>;

    template <template <class...> class AnnotationTemplate>
    static constexpr bool has_annotation_template = (detail::is_specialization<Annos, AnnotationTemplate>::value || ...);
    
    template <class Annotation>
    static constexpr bool has_annotation_class = (std::is_same_v<Annotation, Annos> || ...);

    template <template <class...> class AnnotationTemplate>
    requires (has_annotation_template<AnnotationTemplate>)
    using FindAnnotation = detail::FindAnnotationImpl<AnnotationTemplate, Annos...>::Type;
};

namespace detail {

    template <typename T, std::size_t N>
    struct Tag {
        friend auto loophole(Tag<T, N>);
    };

    template <class T, std::size_t N, class F>
    struct LoopholeSet {
        friend auto loophole(Tag<T, N>) { return F{}; };
    };

    template <class T, std::size_t N>
    using LoopholeGet = decltype(loophole(Tag<T, N>{}));

    template <typename T, std::size_t N>
    struct FieldTag {
        friend auto field_loophole(FieldTag<T, N>);
    };

    template <class T, std::size_t N, class F>
    struct FieldLoopholeSet {
        friend auto field_loophole(FieldTag<T, N>) { return F{}; };
    };

    template <class T, std::size_t N>
    using FieldLoopholeGet = decltype(field_loophole(FieldTag<T, N>{}));

    template <std::size_t I>
    struct UbiqConstructor {
        template <class Type>
        constexpr operator Type() const noexcept {
            return {};
        }
    };

    template <class T, std::size_t I>
    struct UbiqInitialize {
        template <class Type>
        constexpr operator Type() const noexcept {
            [[maybe_unused]] LoopholeSet<T, I, Type> unused{};
            return {};
        }
    };

    template <class T, class... Args>
    concept AggregateConstructibleFrom = requires (Args... args) {
        T{args...};
    };

    template <class Field>
    concept IsAnnotate = IsSpec<Field, Annotate>;

    template <class T, std::size_t... I>
    constexpr std::size_t CountRawFieldsImpl(std::index_sequence<I...>) {
        return sizeof...(I) - 1;
    }

    template <class T, std::size_t... I>
    requires (AggregateConstructibleFrom<T, UbiqConstructor<I>...>)
    constexpr std::size_t CountRawFieldsImpl(std::index_sequence<I...>) {
        return CountRawFieldsImpl<T>(std::index_sequence<0, I...>{});
    }

    template <class T, std::size_t I, std::size_t>
    auto ForceCalculation() {
        return detail::FieldLoopholeGet<T, I>{};
    }

};  // namespace detail

template <class T>
struct Describe {
  private:
    static constexpr std::size_t num_fields_with_annots = detail::CountRawFieldsImpl<T>(std::index_sequence<0>{});

    template <std::size_t I, std::size_t RealIndex, typename... Annots, typename... NewAnnots>
    static constexpr std::size_t AnnotateExpandHelper(Annotate<NewAnnots...>) {
        return CountFieldsImpl<I, RealIndex, Annots..., NewAnnots...>();
    }
    
    template <std::size_t I, std::size_t RealIndex = 0, typename... Annots>
    static constexpr std::size_t CountFieldsImpl() {
        if constexpr (I == num_fields_with_annots) {
            return RealIndex;
        } else {
            if constexpr (detail::IsAnnotate<detail::LoopholeGet<T, I>>) {
                return AnnotateExpandHelper<I + 1, RealIndex, Annots...>(detail::LoopholeGet<T, I>{});
            } else {
                [[maybe_unused]] detail::FieldLoopholeSet<T, RealIndex, FieldDescriptor<T, RealIndex, detail::LoopholeGet<T, I>, Annots...>> unused{};
                return CountFieldsImpl<I + 1, RealIndex + 1>();
            }
        }
    }

    template <std::size_t... I>
    static constexpr std::size_t CountFields(std::index_sequence<I...>) {
        [[maybe_unused]] T unused_{detail::UbiqInitialize<T, I>{}...};
        return CountFieldsImpl<0, 0>();
    }

  public:
    static constexpr std::size_t num_fields = CountFields(std::make_index_sequence<num_fields_with_annots>());
    
    template <std::size_t I>
    using Field = decltype(detail::ForceCalculation<T, I, num_fields>());
};
