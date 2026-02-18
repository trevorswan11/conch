#include <cctype>
#include <ranges>
#include <string>

#include "string.hpp"
#include "types.hpp"

namespace conch {

static_assert(std::is_same_v<std::string::value_type, conch::byte>);

namespace string {

auto is_space(byte b) noexcept -> bool { return std::isspace(b); }

auto trim_left(std::string_view str) noexcept -> std::string_view {
    const auto first = std::ranges::find_if_not(str, is_space);
    return std::string_view{first, static_cast<usize>(str.end() - first)};
}

auto trim_right(std::string_view str) noexcept -> std::string_view {
    const auto last = std::ranges::find_if_not(str | std::views::reverse, isspace).base();
    return std::string_view{str.begin(), last};
}

auto trim(std::string_view str) noexcept -> std::string_view {
    const auto ltrim = trim_left(str);
    return trim_right(ltrim);
}

} // namespace string
} // namespace conch
