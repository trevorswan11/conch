#pragma once

#include <string_view>
#include <type_traits>

// Returns the name of an enum as a string at compile time.
template <auto V> consteval auto enum_name() {
    constexpr auto impl = []<typename E, E EV>() {
        static_assert(std::is_enum_v<E>);
        constexpr std::string_view func{__PRETTY_FUNCTION__};

        constexpr auto before = func.find_last_of(':');
        static_assert(before != std::string_view::npos,
                      "Enums must be marked 'enum class' or be namespaced");

        constexpr auto after = func.find_last_of(']');
        static_assert(after != std::string_view::npos,
                      "The compiler likely changed how pretty function works");

        constexpr auto name = func.substr(before + 1, after - (before + 1));
        static_assert(name.size() > 0);
        return name;
    };

    return impl.template operator()<decltype(V), V>();
}
