#pragma once

#include <vector>

#include <stdx/assert.hh>
#include <stdx/option.hh>
#include <stdx/types.hh>

#include "ast/id.hh"
#include "ast/kind.hh"

namespace ghoti::ast {

namespace detail {

// Checks if the provided kind is compatible with any allowed kind
template <NodeKind... AllowedKinds>
[[nodiscard]] constexpr auto any_kind_compatible(NodeKind kind) noexcept -> bool {
    return ((kind == AllowedKinds) || ...);
}

} // namespace detail

// A pseudo-type-safe wrapper around nodes, defaulting to an invalid state
template <NodeKind... AllowedKinds> class Handle {
  public:
    constexpr explicit Handle(NodeID id) noexcept : id_{id} {
        ASSERT(id.is_valid(), "Attempt to create Handle from invalid NodeID");
        ASSERT(any_compatible(id.get_kind()), "Assigned invalid NodeKind to Handle");
    }

    template <NodeKind... OtherKinds>
        requires(detail::any_kind_compatible<AllowedKinds...>(OtherKinds) || ...)
    constexpr Handle(Handle<OtherKinds...> other) noexcept : id_{*other} {
        ASSERT(other.is_valid(), "Attempt to create Handle from invalid Handle");
    }

    [[nodiscard]] constexpr static auto any_compatible(NodeKind kind) noexcept -> bool {
        return detail::any_kind_compatible<AllowedKinds...>(kind);
    }

    [[nodiscard]] constexpr auto operator*() const noexcept -> NodeID { return id_; }
    [[nodiscard]] constexpr auto operator->(this auto&& self) noexcept { return &self.id_; }

    [[nodiscard]] constexpr auto        is_valid() const noexcept -> bool { return id_.is_valid(); }
    [[nodiscard]] static constexpr auto make_invalid() noexcept -> Handle {
        return Handle{detail::INVALID_ID};
    }

    [[nodiscard]] constexpr auto get_index() const noexcept -> usize { return id_.get_index(); }
    [[nodiscard]] constexpr      operator NodeID() const noexcept { return id_; } // NOLINT

    template <traits::ASTNode N> [[nodiscard]] constexpr auto is() const noexcept -> bool {
        return id_.is<N>();
    }

    template <traits::ASTNode... Ns> [[nodiscard]] constexpr auto any() const noexcept -> bool {
        return id_.any<Ns...>();
    }

  private:
    constexpr explicit Handle(u64 raw) noexcept : id_{raw} {}

  private:
    NodeID id_{NodeID::make_invalid()};
};

using ExpressionHandle = Handle<NodeKind::ARRAY_EXPRESSION,
                                NodeKind::ASSIGNMENT_EXPRESSION,
                                NodeKind::BINARY_EXPRESSION,
                                NodeKind::CALL_EXPRESSION,
                                NodeKind::DO_WHILE_LOOP_EXPRESSION,
                                NodeKind::DOT_EXPRESSION,
                                NodeKind::ENUM_EXPRESSION,
                                NodeKind::FOR_LOOP_EXPRESSION,
                                NodeKind::FUNCTION_EXPRESSION,
                                NodeKind::IDENTIFIER_EXPRESSION,
                                NodeKind::IF_EXPRESSION,
                                NodeKind::INDEX_EXPRESSION,
                                NodeKind::INFINITE_LOOP_EXPRESSION,
                                NodeKind::INITIALIZER_EXPRESSION,
                                NodeKind::LABEL_EXPRESSION,
                                NodeKind::MATCH_EXPRESSION,
                                NodeKind::UNARY_EXPRESSION,
                                NodeKind::REFERENCE_EXPRESSION,
                                NodeKind::DEREFERENCE_EXPRESSION,
                                NodeKind::ADDRESS_OF_EXPRESSION,
                                NodeKind::IMPLICIT_ACCESS_EXPRESSION,
                                NodeKind::STRING_EXPRESSION,
                                NodeKind::I32_EXPRESSION,
                                NodeKind::I64_EXPRESSION,
                                NodeKind::ISIZE_EXPRESSION,
                                NodeKind::U32_EXPRESSION,
                                NodeKind::U64_EXPRESSION,
                                NodeKind::USIZE_EXPRESSION,
                                NodeKind::U8_EXPRESSION,
                                NodeKind::F32_EXPRESSION,
                                NodeKind::F64_EXPRESSION,
                                NodeKind::BOOL_EXPRESSION,
                                NodeKind::VOID_EXPRESSION,
                                NodeKind::UNDEFINED_EXPRESSION,
                                NodeKind::RANGE_EXPRESSION,
                                NodeKind::MODULE_ACCESS_EXPRESSION,
                                NodeKind::STRUCT_EXPRESSION,
                                NodeKind::UNION_EXPRESSION,
                                NodeKind::WHILE_LOOP_EXPRESSION>;

using IdentifierHandle       = Handle<NodeKind::IDENTIFIER_EXPRESSION>;
using DiscardableIdentHandle = Handle<NodeKind::IDENTIFIER_EXPRESSION, NodeKind::DISCARDED>;
using ImplicitAccessHandle   = Handle<NodeKind::IMPLICIT_ACCESS_EXPRESSION>;
using StringHandle           = Handle<NodeKind::STRING_EXPRESSION>;
using OuterAccessHandle      = Handle<NodeKind::IDENTIFIER_EXPRESSION,
                                      NodeKind::MODULE_ACCESS_EXPRESSION,
                                      NodeKind::DOT_EXPRESSION>;

using MatchPatternHandle = Handle<NodeKind::CALL_EXPRESSION,
                                  NodeKind::DOT_EXPRESSION,
                                  NodeKind::IDENTIFIER_EXPRESSION,
                                  NodeKind::INDEX_EXPRESSION,
                                  NodeKind::UNARY_EXPRESSION,
                                  NodeKind::REFERENCE_EXPRESSION,
                                  NodeKind::DEREFERENCE_EXPRESSION,
                                  NodeKind::ADDRESS_OF_EXPRESSION,
                                  NodeKind::IMPLICIT_ACCESS_EXPRESSION,
                                  NodeKind::STRING_EXPRESSION,
                                  NodeKind::I32_EXPRESSION,
                                  NodeKind::I64_EXPRESSION,
                                  NodeKind::ISIZE_EXPRESSION,
                                  NodeKind::U32_EXPRESSION,
                                  NodeKind::U64_EXPRESSION,
                                  NodeKind::USIZE_EXPRESSION,
                                  NodeKind::U8_EXPRESSION,
                                  NodeKind::F32_EXPRESSION,
                                  NodeKind::F64_EXPRESSION,
                                  NodeKind::BOOL_EXPRESSION,
                                  NodeKind::MODULE_ACCESS_EXPRESSION,
                                  NodeKind::DISCARDED>;

using StatementHandle = Handle<NodeKind::BLOCK_STATEMENT,
                               NodeKind::DECL_STATEMENT,
                               NodeKind::DEFER_STATEMENT,
                               NodeKind::DISCARD_STATEMENT,
                               NodeKind::EXPRESSION_STATEMENT,
                               NodeKind::IMPORT_STATEMENT,
                               NodeKind::RETURN_STATEMENT,
                               NodeKind::BREAK_STATEMENT,
                               NodeKind::CONTINUE_STATEMENT,
                               NodeKind::TEST_STATEMENT,
                               NodeKind::USING_STATEMENT>;

struct DeclStatement;
using DeclHandle          = Handle<NodeKind::DECL_STATEMENT>;
using BlockHandle         = Handle<NodeKind::BLOCK_STATEMENT>;
using ImportHandle        = ast::Handle<ast::NodeKind::IMPORT_STATEMENT>;
using ImportPayloadHandle = Handle<NodeKind::STRING_EXPRESSION, NodeKind::IDENTIFIER_EXPRESSION>;

using MemberHandle =
    Handle<NodeKind::DECL_STATEMENT, NodeKind::IMPORT_STATEMENT, NodeKind::USING_STATEMENT>;
using Members = std::vector<MemberHandle>;

using LabeledNodeHandle = Handle<NodeKind::DO_WHILE_LOOP_EXPRESSION,
                                 NodeKind::FOR_LOOP_EXPRESSION,
                                 NodeKind::IF_EXPRESSION,
                                 NodeKind::INFINITE_LOOP_EXPRESSION,
                                 NodeKind::MATCH_EXPRESSION,
                                 NodeKind::WHILE_LOOP_EXPRESSION,
                                 NodeKind::BLOCK_STATEMENT>;

} // namespace ghoti::ast

namespace stdx {

template <ghoti::ast::NodeKind... Kinds> struct nullable<ghoti::ast::Handle<Kinds...>> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ghoti::ast::Handle<Kinds...> {
        return ghoti::ast::Handle<Kinds...>::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(ghoti::ast::Handle<Kinds...> handle) noexcept
        -> bool {
        return handle.is_valid();
    }
};

} // namespace stdx
