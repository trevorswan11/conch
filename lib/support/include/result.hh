#pragma once

#include <concepts>
#include <expected>
#include <type_traits>

#include "variant.hh"

namespace ghoti {

template <typename E> using Err = std::__1::unexpected<E>;

namespace detail {

// A result type that has no associated value
template <typename E> class EmptyResult {
  public:
    // cppcheck-suppress-begin noExplicitConstructor
    constexpr EmptyResult() noexcept = default;

    // Constructs the error type in place
    template <typename... Args> constexpr EmptyResult(Args&&... args) {
        error_.template emplace<E>(std::forward<Args>(args)...);
    }

    constexpr EmptyResult(Err<E>&& err) : error_{std::move(err.error())} {}
    // cppcheck-suppress-end noExplicitConstructor

    // Checks for the lack of presence of the underlying error, mirrors std::expected
    [[nodiscard]] constexpr auto has_value() const noexcept -> bool { return !has_error(); }
    constexpr auto               value() const -> void {}
    constexpr auto               operator*() const -> void { return value(); }

    [[nodiscard]] constexpr auto has_error() const noexcept -> bool {
        return error_.template is<E>();
    }
    [[nodiscard]] constexpr auto error(this auto&& self) -> decltype(auto) {
        return self.error_.template as<E>();
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    template <typename Or = E> constexpr auto error_or(Or&& or_value) const& {
        // This is straight from clang's stdc++ C++23 expected implementation
        static_assert(std::is_copy_constructible_v<Or>, "error_type has to be copy constructible");
        static_assert(std::is_convertible_v<Or, E>, "argument has to be convertible to error_type");
        if (has_value()) { return std::forward<Or>(or_value); }
        return error();
    }

    [[nodiscard]] constexpr auto operator==(const EmptyResult&) const noexcept -> bool = default;

  private:
    Variant<Unit, E> error_;
};

// Uses explicit inline namespace due to name collisions in std
template <typename T, typename E> using ValuedResult = std::__1::expected<T, E>;

template <typename T, typename E> struct ResultImpl {
    using type = ValuedResult<T, E>;
};

template <std::same_as<void> T, typename E> struct ResultImpl<T, E> {
    using type = EmptyResult<E>;
};

} // namespace detail

template <typename T, typename E> using Result = detail::ResultImpl<T, E>::type;

template <typename E, typename... Args>
[[nodiscard]] constexpr auto make_err(Args&&... args) -> Err<E> {
    return Err<E>{E{std::forward<Args>(args)...}};
}

// A hack to imitate the 'try' keyword in zig using GNU Statement Expressions
// https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define TRY(expr)                                                   \
    ({                                                              \
        auto&& _e = (expr);                                         \
        if (!_e.has_value()) { return Err{std::move(_e).error()}; } \
        std::move(_e).value();                                      \
    })

namespace traits {

template <typename T> struct is_result : std::false_type {};
template <typename T, typename E> struct is_result<detail::ValuedResult<T, E>> : std::true_type {};
template <typename E> struct is_result<detail::EmptyResult<E>> : std::true_type {};
template <typename T> constexpr bool is_result_v = is_result<T>::value;

template <typename T>
concept Result = is_result_v<T>;

} // namespace traits

} // namespace ghoti
