#pragma once

#include <concepts>
#include <vector>

#include <stdx/assert.hh>
#include <stdx/option.hh>

#include "ast/ast.hh"
#include "ast/id.hh"
#include "ast/traits.hh"

namespace ghoti::sema {

namespace detail {

// An ID-indexable side table containing attached data
template <traits::IndexableID ID, stdx::traits::Option T> struct SideTable {
    std::vector<T> values;

    // Allows a handle wrapper of a node to be used for raw ID-based tables
    template <typename U>
        requires std::convertible_to<U, ID>
    [[nodiscard]] constexpr auto operator[](this auto&& self, U id) noexcept -> auto& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        return self.values[static_cast<ID>(id).get_index()];
    }
};

} // namespace detail

class Type;

struct SideTables {
    detail::SideTable<ast::NodeID, stdx::Option<sema::Type&>>         node_types;
    detail::SideTable<ast::ExplicitTypeID, stdx::Option<sema::Type&>> explicit_types;

    // Indexed by the match arms' pattern
    detail::SideTable<ast::NodeID, stdx::Option<sema::Type&>> match_arm_types;

    // Allocates `size` slots in all backing vectors
    constexpr auto resize(const ast::AST::DataPoolSizes& sizes) -> void {
        node_types.values.resize(sizes.nodes_size);
        explicit_types.values.resize(sizes.types_size);
        match_arm_types.values.resize(sizes.nodes_size);
    }
};

} // namespace ghoti::sema
