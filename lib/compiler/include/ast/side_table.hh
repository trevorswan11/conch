#pragma once

#include <concepts>
#include <vector>

#include "ast/traits.hh"

#include "assert.hh"
#include "type_traits.hh"

namespace porpoise::ast {

// An ID-indexable side table containing attached data
template <traits::IndexableID ID, traits::DefaultConstructible T> struct SideTable {
    std::vector<T> values;

    // Allows a handle wrapper of a node to be used for raw ID-based tables
    template <typename U>
        requires(std::convertible_to<U, ID>)
    [[nodiscard]] constexpr auto operator[](this auto&& self, U id) noexcept -> auto& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return self.values[static_cast<ID>(id).get_index()];
    }
};

} // namespace porpoise::ast
