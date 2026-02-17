#pragma once

#include <string_view>
#include <type_traits>
#include <utility>

namespace conch {

// Returns the name of an enum as a string at compile time.
template <auto V> consteval auto enum_name() noexcept -> std::string_view {
    return []<typename E, E EV>() -> std::string_view {
        static_assert(std::is_scoped_enum_v<E>);
        constexpr std::string_view func{__PRETTY_FUNCTION__};

        constexpr auto before = func.find_last_of(':');
        static_assert(before != std::string_view::npos);

        constexpr auto name = func.substr(before + 1, func.size() - before - 2);
        static_assert(name.size() > 0);
        return name;
    }.template operator()<decltype(V), V>();
}

// Returns the name of an enum as a string at runtime.
template <typename E, int Min = 0, int Max = 255>
    requires(std::is_scoped_enum_v<E>)
auto enum_name(E value) noexcept -> std::string_view {
    using U = std::underlying_type_t<E>;
    using A = std::array<std::string_view, Max - Min>;

    static constexpr auto names = []() -> A {
        A arr{};
        arr.fill("UNKNOWN");

        [&arr]<U... Is>(std::integer_sequence<U, Is...>) {
            ((arr[Is] = enum_name<static_cast<E>(Is + Min)>()), ...);
        }(std::make_integer_sequence<U, Max - Min>{});

        return arr;
    }();

    const U idx = static_cast<U>(value);
    return (idx < Min || idx >= Max) ? "UNKNOWN" : names[idx - Min];
}

} // namespace conch
