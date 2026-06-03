#pragma once

#include <array>
#include <cstddef>

#include "type_traits.hh"
#include "types.hh"

namespace ghoti::fixed::detail {

// Non-default constructible object use a raw byte array initialized on the fly
template <typename Data, usize Capacity> struct Storage {
    // Operations on this byte array are generally non constexpr-capable
    alignas(Data) std::byte items[Capacity * sizeof(Data)];

    template <typename Self> [[nodiscard]] auto data(this Self&& self) noexcept {
        return reinterpret_cast<traits::const_dispatch_t<Self, Data>*>(self.items);
    }
};

// Default constructible objects can be freely constructed all at once
template <traits::DefaultConstructible Data, usize Capacity> struct Storage<Data, Capacity> {
    std::array<Data, Capacity> items{};

    [[nodiscard]] constexpr auto data(this auto&& self) noexcept -> auto* {
        return self.items.data();
    }
};

} // namespace ghoti::fixed::detail
