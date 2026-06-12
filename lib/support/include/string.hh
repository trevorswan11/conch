#pragma once

#include <algorithm>
#include <cctype>
#include <concepts>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>

#include "types.hh"

namespace ghoti {

namespace traits {

template <typename T> struct is_string_like : std::false_type {};
template <> struct is_string_like<std::string> : std::true_type {};
template <> struct is_string_like<std::string_view> : std::true_type {};
template <> struct is_string_like<const char*> : std::true_type {};
template <> struct is_string_like<char*> : std::true_type {};

template <typename T>
concept StringLike = is_string_like<std::remove_cvref_t<T>>::value;

} // namespace traits

namespace string {

template <typename Func>
concept Predicate = std::convertible_to<std::invoke_result_t<Func, char>, bool>;

// Trim all characters that fulfill the predicate from the left of the string
template <Predicate Pred>
[[nodiscard]] constexpr auto trim_left(
    std::string_view str, Pred pred = [](char c) -> bool { return std::isspace(c); }) noexcept
    -> std::string_view {
    const auto first{std::ranges::find_if_not(str, pred)};
    return std::string_view{first, static_cast<usize>(str.end() - first)};
}

// Trims leftmost spaces
[[nodiscard]] constexpr auto trim_left(std::string_view str) noexcept -> std::string_view {
    return trim_left(str, [](char c) -> bool { return std::isspace(c); });
}

// Trim all characters that fulfill the predicate from the right of the string
template <Predicate Pred>
[[nodiscard]] constexpr auto trim_right(std::string_view str, Pred pred) noexcept
    -> std::string_view {
    const auto last{std::ranges::find_if_not(str | std::views::reverse, pred).base()};
    return std::string_view{str.begin(), last};
}

// Trims rightmost spaces
[[nodiscard]] constexpr auto trim_right(std::string_view str) noexcept -> std::string_view {
    return trim_right(str, [](char c) -> bool { return std::isspace(c); });
}

// Trim all characters that fulfill the predicate from both ends of the string
template <Predicate Pred>
[[nodiscard]] constexpr auto trim(std::string_view str, Pred pred) noexcept -> std::string_view {
    const auto ltrim{trim_left(str, pred)};
    return trim_right(ltrim, pred);
}

// Trims both ends' spaces
[[nodiscard]] constexpr auto trim(std::string_view str) noexcept -> std::string_view {
    return trim(str, [](char c) -> bool { return std::isspace(c); });
}

// Zero allocation substring returning empty substring for invalid input
[[nodiscard]] constexpr auto
substr(std::string_view str, usize pos, usize len = std::string_view::npos) noexcept
    -> std::string_view {
    return pos > str.size() ? std::string_view{}
                            : std::string_view{str.data() + pos, std::min(len, str.size() - pos)};
}

// Think hard about why a view of an rvalue temporary string is a bad idea
auto substr(std::string&& str, usize pos, usize len = std::string_view::npos) noexcept
    -> std::string_view = delete;

// Checks if the provided string contains entirely whitespace characters
[[nodiscard]] constexpr auto is_blank(std::string_view text) -> bool {
    return std::ranges::all_of(text, [](char c) -> bool { return std::isspace(c); });
}

// Converts a string-like object to its string_view representation
template <traits::StringLike S>
[[nodiscard]] constexpr auto to_view(const S& input) noexcept -> std::string_view {
    return input;
}

} // namespace string

} // namespace ghoti
