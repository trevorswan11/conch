#pragma once

#include <algorithm>

#include "types.hpp"

namespace conch {

// Materializes a sized view into its corresponding array representation
template <auto N, typename Range> constexpr auto materialize_sized_view(Range&& r) {
    std::array<std::ranges::range_value_t<Range>, N> arr{};
    std::ranges::copy(r, arr.begin());
    return arr;
}

template <typename T, usize... Ns>
constexpr auto concat_arrays(const std::array<T, Ns>&... arrays) {
    std::array<T, (Ns + ...)> result{};
    usize                     offset{};
    ((std::ranges::copy(arrays, result.begin() + offset), offset += Ns), ...);
    return result;
}

} // namespace conch
