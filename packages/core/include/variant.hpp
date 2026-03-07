#pragma once

#include <variant> // IWYU pragma: export

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

#define VARIANT_UNPACKER(name, ReturnType, InnerType, member, getter)     \
    [[nodiscard]] auto get_##name() const noexcept -> const ReturnType& { \
        try {                                                             \
            return getter<InnerType>(member);                             \
        } catch (...) { std::unreachable(); }                             \
    }                                                                     \
                                                                          \
    [[nodiscard]] auto is_##name() const noexcept -> bool {               \
        return std::holds_alternative<InnerType>(member);                 \
    }
