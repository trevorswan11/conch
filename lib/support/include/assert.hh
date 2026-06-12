#pragma once

#include <exception>
#include <iostream>
#include <source_location>
#include <string_view>

#include <fmt/ostream.h>

namespace ghoti::detail {

constexpr auto assert_impl(std::source_location loc,
                           bool                 condition,
                           std::string_view     message,
                           std::string_view     expression) -> void {
    if (!condition) {
        if consteval { // cppcheck-suppress syntaxError
            throw "Compile-time assertion failed";
        } else {
            fmt::println(std::cerr,
                         "{}{}{}{}: {}:{}:{}",
                         message,
                         message.empty() ? "" : " (",
                         expression,
                         message.empty() ? "" : ")",
                         loc.file_name(),
                         loc.line(),
                         loc.column());
            std::terminate();
        }
    }
}

constexpr auto unreachable_impl(std::source_location loc, std::string_view message) -> void {
    if consteval { // cppcheck-suppress syntaxError
        throw "Unreachable code reached at compile time";
    } else {
        fmt::println(std::cerr, "{}: {}:{}:{}", message, loc.file_name(), loc.line(), loc.column());
    }
}

} // namespace ghoti::detail

// VERIFY_* macros are not compiled out in release builds
#define VERIFY_1(expression)      \
    ::ghoti::detail::assert_impl( \
        std::source_location::current(), static_cast<bool>(expression), "", #expression)
#define VERIFY_2(expression, message) \
    ::ghoti::detail::assert_impl(     \
        std::source_location::current(), static_cast<bool>(expression), (message), #expression)

#define GET_VERIFY_MACRO(_1, _2, NAME, ...) NAME
#define VERIFY(...) GET_VERIFY_MACRO(__VA_ARGS__, VERIFY_2, VERIFY_1)(__VA_ARGS__)

#ifndef NDEBUG
#    define ASSERT(...) GET_VERIFY_MACRO(__VA_ARGS__, VERIFY_2, VERIFY_1)(__VA_ARGS__)

#    define UNREACHABLE(message)                                                       \
        ::ghoti::detail::unreachable_impl(std::source_location::current(), (message)); \
        std::unreachable()
#else
#    define ASSERT_1(expression)                         \
        do {                                             \
            if constexpr (false) { (void)(expression); } \
        } while (0)
#    define ASSERT_2(expression, message) ASSERT_1(expression)
#    define GET_ASSERT_MACRO(_1, _2, NAME, ...) NAME
#    define ASSERT(...) GET_ASSERT_MACRO(__VA_ARGS__, ASSERT_2, ASSERT_1)(__VA_ARGS__)

#    define UNREACHABLE(message) std::unreachable()
#endif
