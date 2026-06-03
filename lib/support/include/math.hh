#pragma once

#include "type_traits.hh"

namespace ghoti {

// Returns the rounded-up power of two given the unsigned value
template <traits::Unsigned U> [[nodiscard]] constexpr auto ceil_power_of_two(U val) noexcept -> U {
    // If it's already a power of two there's no need to round
    if (val == 0) {
        return 1;
    } else if ((val & (val - 1)) == 0) {
        return val;
    }

    // https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
    val |= --val >> 1;
    val |= val >> 2;
    val |= val >> 4;

    if constexpr (sizeof(U) >= 2) { val |= val >> 8; }
    if constexpr (sizeof(U) >= 4) { val |= val >> 16; }
    if constexpr (sizeof(U) == 8) { val |= val >> 32; }

    return ++val;
}

template <traits::Unsigned U> [[nodiscard]] constexpr auto is_power_of_two(U val) noexcept -> bool {
    return (val > 0) && ((val & (val - 1)) == 0);
}

} // namespace ghoti
