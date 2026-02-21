#pragma once

#include <algorithm>
#include <utility>
#include <array>

#include "types.hpp"

namespace conch::array {

// Materializes a sized view into its corresponding array representation
template <auto N, typename Range> [[nodiscard]] constexpr auto materialize_sized_view(Range&& r) {
    std::array<std::ranges::range_value_t<Range>, N> arr{};
    std::ranges::copy(r, arr.begin());
    return arr;
}

template <typename T, usize... Ns>
[[nodiscard]] constexpr auto concat(const std::array<T, Ns>&... arrays) {
    std::array<T, (Ns + ...)> result{};
    usize                     offset{};
    ((std::ranges::copy(arrays, result.begin() + offset), offset += Ns), ...);
    return result;
}

// Calculates the array's combinations with itself, given by:
// [A, B, C] -> [[A, B], [A, C], [B, B]]
template <typename T, usize N> auto combinations(std::array<T, N> input) {
    constexpr auto                    size = (N * (N - 1)) / 2;
    std::array<std::pair<T, T>, size> results;
    if (input.size() < 2) { return results; }

    for (usize i = 0, idx = 0; i < input.size() - 1; ++i) {
        for (usize j = i + 1; j < input.size(); ++j) { results[idx++] = {input[i], input[j]}; }
    }
    return results;
}

} // namespace conch::array
