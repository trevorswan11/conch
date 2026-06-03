#pragma once

#include <vector>

#include "ast/id.hh"
#include "ast/side_table.hh"

#include "option.hh"
#include "types.hh"

namespace porpoise::sema {

class Type;

struct SideTables {
    ast::SideTable<ast::NodeID, opt::Option<sema::Type&>>         node_types;
    ast::SideTable<ast::ExplicitTypeID, opt::Option<sema::Type&>> explicit_types;

    // Indexed by the match arms' pattern
    ast::SideTable<ast::NodeID, opt::Option<sema::Type&>> match_arm_types;

    // Allocates `size` slots in all backing vectors
    constexpr auto resize(usize size) -> void {
        node_types.values.resize(size);
        explicit_types.values.resize(size);
        match_arm_types.values.resize(size);
    }
};

} // namespace porpoise::sema
