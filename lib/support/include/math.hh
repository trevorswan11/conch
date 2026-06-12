#pragma once

#include <type_traits>

#include "type_traits.hh"
#include "types.hh"

namespace ghoti {

// Returns the rounded-up power of two given the unsigned value
template <traits::Unsigned U> [[nodiscard]] constexpr auto ceil_power_of_two(U val) noexcept -> U {
    // If it's already a power of two there's no need to round
    if (val == 0) { return 1; }
    if ((val & (val - 1)) == 0) { return val; }

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

// The minimum number of bits required to hold the provided value
template <auto U>
constexpr auto min_bits = [] -> auto {
    auto  value{U};
    usize bits{0};
    while (value > 0) {
        bits++;
        value >>= 1;
    }
    return bits == 0 ? 1 : bits;
}();

namespace traits {

// https://stackoverflow.com/questions/74244055/in-c-get-smallest-integer-type-that-can-hold-given-amount-of-bits
template <usize Bits>
using min_uint_for_bits = std::conditional_t<
    Bits <= 8,
    u8,
    std::conditional_t<Bits <= 16, u16, std::conditional_t<Bits <= 32, u32, u64>>>;

} // namespace traits

} // namespace ghoti
