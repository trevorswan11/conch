#pragma once

#include <cassert>
#include <cstdio>
#include <optional>
#include <string_view>

#include <fmt/format.h>

namespace conch {

namespace detail {

template <typename... Args>
auto todo_impl(std::optional<std::string_view> message) noexcept -> void {
    if (message) { fmt::println(stderr, "TODO: {}", *message); }
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
