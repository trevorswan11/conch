#pragma once

#include <cassert>
#include <cstdio>
#include <optional>
#include <print>
#include <string_view>

namespace conch {

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

namespace detail {

template <typename... Args>
auto todo_impl(std::optional<std::string_view> message) noexcept -> void {
    if (message) { std::println(stderr, "TODO: {}", *message); }
    assert(false);
}

template <typename... Args> auto todo([[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(std::nullopt);
}

template <typename... Args>
auto todo(std::string_view message, [[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(message);
}

} // namespace detail

#define TODO(...)              \
    detail::todo(__VA_ARGS__); \
    std::unreachable()

} // namespace conch
