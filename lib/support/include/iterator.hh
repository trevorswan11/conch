#pragma once

#include <tuple>
#include <type_traits>

#include "type_traits.hh"
#include "types.hh"

namespace ghoti {

// Similar to a std::pair, but the Visitor may be a function pointer
template <typename Iterable, typename Visitor> struct IterPair {
    const Iterable& iterable;
    Visitor         visitor;
};

namespace traits {

template <typename Self, typename T> using data_pointer_t = const_dispatch_t<Self, T>*;

template <typename T>
concept InsertablePair = requires {
    typename std::tuple_element_t<0, std::remove_cvref_t<T>>;
    typename std::tuple_element_t<1, std::remove_cvref_t<T>>;
    requires std::tuple_size_v<std::remove_cvref_t<T>> >= 2;
};

template <usize I, typename... Ts>
using common_tuple_type_t = std::common_type_t<std::tuple_element_t<I, std::remove_cvref_t<Ts>>...>;

} // namespace traits

// Gives the enclosing type an iterator interface based on an iterator-capable type
#define MAKE_UNALIASED_ITERATOR(Container, member)                                                \
    using iterator       = typename Container::iterator;                                          \
    using const_iterator = typename Container::const_iterator;                                    \
                                                                                                  \
    [[nodiscard]] constexpr auto begin(this auto&& self) noexcept { return self.member.begin(); } \
    [[nodiscard]] constexpr auto end(this auto&& self) noexcept { return self.member.end(); }     \
                                                                                                  \
    [[nodiscard]] constexpr auto size() const noexcept -> usize { return (member).size(); }       \
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return (member).empty(); }

// NOLINTBEGIN
// Gives the enclosing type an iterator interface and type alias based on an iterator-capable type
#define MAKE_ITERATOR(Alias, Container, member) \
    using Alias = Container;                    \
    MAKE_UNALIASED_ITERATOR(Alias, member)
// NOLINTEND

} // namespace ghoti
