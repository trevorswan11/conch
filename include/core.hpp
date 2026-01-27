#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <expected>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "util/optional.hpp"

using u8    = uint8_t;
using i8    = uint8_t;
using u16   = uint16_t;
using i16   = uint16_t;
using u32   = uint32_t;
using i32   = uint32_t;
using u64   = uint64_t;
using i64   = uint64_t;
using usize = size_t;
using isize = std::make_signed<usize>;

using f32 = float;
using f64 = double;

using byte = std::string_view::value_type;
static_assert(std::is_same_v<std::string::value_type, byte>);

template <typename T, typename E> using Expected = std::__1::expected<T, E>;
template <typename E> using Unexpected           = std::__1::unexpected<E>;

#define TRY(expr)                                                      \
    ({                                                                 \
        auto&& _e = (expr);                                            \
        if (!_e.has_value()) return Unexpected{std::move(_e).error()}; \
        std::move(_e).value();                                         \
    })

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
template <typename E, int Min = 0, int Max = 256>
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

template <typename E>
    requires std::is_scoped_enum_v<E>
class Diagnostic {
  public:
    Diagnostic() = delete;
    explicit Diagnostic(E err) : error_{err} {}
    explicit Diagnostic(E err, usize ln, usize col) : error_{err}, line_{ln}, column_{col} {}
    explicit Diagnostic(std::string msg, E err, usize ln, usize col)
        : message_{std::move(msg)}, error_{err}, line_{ln}, column_{col} {}

    template <typename T>
        requires requires(T t) {
            t.line;
            t.column;
        }
    explicit Diagnostic(std::string msg, E err, T t)
        : message_{std::move(msg)}, error_{err}, line_{t.line}, column_{t.column} {}

    template <typename T>
        requires requires(T t) {
            t.line;
            t.column;
        }
    explicit Diagnostic(E err, T t) : error_{err}, line_{t.line}, column_{t.column} {}

    explicit Diagnostic(Diagnostic& other, E err) noexcept
        : message_{std::move(other.message_)}, error_{err}, line_{other.line_},
          column_{other.column_} {}

    auto message() const noexcept -> const std::string& { return message_; }
    auto error() const noexcept -> E { return error_; }
    auto set_err(E err) noexcept -> void { error_ = err; }

    auto operator==(const Diagnostic& other) const noexcept -> bool {
        return message_ == other.message_ && error_ == other.error_ && line_ == other.line_ &&
               column_ == other.column_;
    }

  private:
    std::string     message_{};
    E               error_;
    Optional<usize> line_{};
    Optional<usize> column_{};

    friend struct std::formatter<Diagnostic>;
};

template <typename E> struct std::formatter<Diagnostic<E>> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const Diagnostic<E>& d, F& ctx) const {
        if (d.line_ && d.column_) {
            return std::formatter<std::string>::format(
                std::format("{} [{}, {}]",
                            d.message_.empty() ? enum_name(d.error_) : d.message_,
                            *d.line_,
                            *d.column_),
                ctx);
        }

        return std::formatter<std::string>::format(
            std::format("{}", d.message_.empty() ? enum_name(d.error_) : d.message_), ctx);
    }
};

template <typename... Args> auto todo_impl(Optional<std::string_view> message) noexcept -> void {
    if (message) { std::cerr << "TODO: " << *message << "\n"; }
    assert(false);
}

template <typename... Args> auto todo([[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(nullopt);
}

template <typename... Args>
auto todo(std::string_view message, [[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(message);
}

#define TODO(...)      \
    todo(__VA_ARGS__); \
    std::unreachable()
