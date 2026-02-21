#pragma once

#include <string_view>

#include "types.hpp"

namespace conch::string {

[[nodiscard]] auto is_space(byte b) noexcept -> bool;

[[nodiscard]] auto trim_left(std::string_view str, bool (*pred)(byte) = is_space) noexcept
    -> std::string_view;
[[nodiscard]] auto trim_right(std::string_view str, bool (*pred)(byte) = is_space) noexcept
    -> std::string_view;
[[nodiscard]] auto trim(std::string_view str, bool (*pred)(byte) = is_space) noexcept
    -> std::string_view;

} // namespace conch::string
