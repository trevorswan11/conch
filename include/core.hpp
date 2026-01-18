#pragma once

#include <array>
#include <flat_map>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

template <typename Pair>
using flat_map_from_pair = std::flat_map<typename Pair::first_type, typename Pair::second_type>;

using byte = std::string_view::value_type;
static_assert(std::is_same_v<std::string::value_type, byte>);

// Returns the name of an enum as a string at compile time.
template <auto V> consteval auto enum_name() noexcept -> std::string_view {
    return []<typename E, E EV>() {
        static_assert(std::is_enum_v<E>);
        constexpr std::string_view func{__PRETTY_FUNCTION__};

        constexpr auto before = func.find_last_of(':');
        static_assert(before != std::string_view::npos,
                      "Enums must be marked 'enum class' or be namespaced");

        constexpr auto name = func.substr(before + 1, func.size() - before - 2);
        static_assert(name.size() > 0);
        return name;
    }.template operator()<decltype(V), V>();
}

// Returns the name of an enum as a string at runtime.
template <typename E, int Min = 0, int Max = 256>
auto enum_name(E value) noexcept -> std::string_view {
    static_assert(std::is_enum_v<E>);
    using U = std::underlying_type_t<E>;

    static constexpr auto names = []() {
        std::array<std::string_view, Max - Min> arr{};
        arr.fill("UNKNOWN");

        [&arr]<U... Is>(std::integer_sequence<U, Is...>) {
            ((arr[Is] = enum_name<static_cast<E>(Is + Min)>()), ...);
        }(std::make_integer_sequence<U, Max - Min>{});

        return arr;
    }();

    const U idx = static_cast<U>(value);
    return (idx < Min || idx >= Max) ? "UNKNOWN" : names[idx - Min];
}
