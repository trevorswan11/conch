#pragma once

#include <exception>
#include <iostream>
#include <source_location>
#include <utility> // IWYU pragma: export

#include <fmt/ostream.h>

namespace ghoti {

// NOLINTBEGIN

#define MAKE_GETTER_2(name, ReturnType) \
    [[nodiscard]] auto get_##name() const noexcept -> ReturnType { return name##_; }
#define MAKE_GETTER_3(name, ReturnType, getter) \
    [[nodiscard]] auto get_##name() const noexcept -> ReturnType { return getter(name##_); }

#define GET_GETTER_MACRO(_1, _2, _3, NAME, ...) NAME
#define MAKE_GETTER(...) GET_GETTER_MACRO(__VA_ARGS__, MAKE_GETTER_3, MAKE_GETTER_2)(__VA_ARGS__)

#define MAKE_DEDUCING_1(name) \
    [[nodiscard]] auto get_##name(this auto&& self) noexcept -> auto& { return self.name##_; }
#define MAKE_DEDUCING_2(name, getter)                                   \
    [[nodiscard]] auto get_##name(this auto&& self) noexcept -> auto& { \
        return getter(self.name##_);                                    \
    }

#define GET_DEDUCING_GETTER_MACRO(_1, _2, NAME, ...) NAME
#define MAKE_DEDUCING_GETTER(...) \
    GET_DEDUCING_GETTER_MACRO(__VA_ARGS__, MAKE_DEDUCING_2, MAKE_DEDUCING_1)(__VA_ARGS__)

#define MAKE_MOVE_CONSTRUCTABLE_ONLY(Type)        \
    Type(const Type&)                  = delete;  \
    auto operator=(const Type&)->Type& = delete;  \
    Type(Type&&) noexcept              = default; \
    auto operator=(Type&&)->Type&      = delete;

#define MAKE_MOVE_ONLY(Type)                            \
    Type(const Type&)                        = delete;  \
    auto operator=(const Type&)->Type&       = delete;  \
    Type(Type&&) noexcept                    = default; \
    auto operator=(Type&&) noexcept -> Type& = default;

#define CONCAT_INNER(a, b) a##b
#define CONCAT(a, b) CONCAT_INNER(a, b)

// NOLINTEND

[[nodiscard]] auto is_tty() noexcept -> bool;

namespace detail {

template <typename... Args>
constexpr auto todo_impl(std::source_location loc, [[maybe_unused]] Args&&... args) noexcept
    -> void {
    fmt::println(std::cerr, "TODO: {}:{}:{}", loc.file_name(), loc.line(), loc.column());
    std::terminate();
}

} // namespace detail

#define TODO(...) ::ghoti::detail::todo_impl(std::source_location::current(), __VA_ARGS__);

} // namespace ghoti
