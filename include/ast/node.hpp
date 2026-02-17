#pragma once

#include <cassert>
#include <concepts>
#include <format>
#include <string>
#include <utility>

#include "lexer/token.hpp"

#include "util/common.hpp"

namespace conch { class Visitor; } // namespace conch

namespace conch::ast {

enum class NodeKind : u8 {
    ARRAY_EXPRESSION,
    ASSIGNMENT_EXPRESSION,
    BINARY_EXPRESSION,
    CALL_EXPRESSION,
    DO_WHILE_LOOP_EXPRESSION,
    DOT_EXPRESSION,
    ENUM_EXPRESSION,
    FOR_LOOP_EXPRESSION,
    FUNCTION_EXPRESSION,
    IDENTIFIER_EXPRESSION,
    IF_EXPRESSION,
    INDEX_EXPRESSION,
    INFINITE_LOOP_EXPRESSION,
    MATCH_EXPRESSION,
    PREFIX_EXPRESSION,
    STRING_EXPRESSION,
    SIGNED_INTEGER_EXPRESSION,
    SIGNED_LONG_INTEGER_EXPRESSION,
    ISIZE_INTEGER_EXPRESSION,
    UNSIGNED_INTEGER_EXPRESSION,
    UNSIGNED_LONG_INTEGER_EXPRESSION,
    USIZE_INTEGER_EXPRESSION,
    BYTE_EXPRESSION,
    FLOAT_EXPRESSION,
    BOOL_EXPRESSION,
    VOID_EXPRESSION,
    RANGE_EXPRESSION,
    SCOPE_RESOLUTION_EXPRESSION,
    STRUCT_EXPRESSION,
    TYPE_EXPRESSION,
    WHILE_LOOP_EXPRESSION,

    BLOCK_STATEMENT,
    DECL_STATEMENT,
    DISCARD_STATEMENT,
    EXPRESSION_STATEMENT,
    IMPORT_STATEMENT,
    JUMP_STATEMENT,
};

class Node;

// A type that can be anything in the Node inheritance hierarchy
template <typename N>
concept NodeSubtype = std::derived_from<N, Node>;

// A necessarily instantiable Node, meaning it has a NodeKind marker.
template <typename T>
concept LeafNode = NodeSubtype<T> && requires {
    { T::KIND } -> std::convertible_to<NodeKind>;
};

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = default;

    Node(const Node&)                = delete;
    Node& operator=(const Node&)     = delete;
    Node(Node&&) noexcept            = default;
    Node& operator=(Node&&) noexcept = delete;

    virtual auto accept(Visitor& v) const -> void = 0;

    auto get_token() const noexcept -> const Token& { return start_token_; }
    auto get_kind() const noexcept -> NodeKind { return kind_; }

    friend auto operator==(const Node& lhs, const Node& rhs) noexcept -> bool {
        if (lhs.kind_ != rhs.kind_) { return false; }
        if (lhs.start_token_.type != rhs.start_token_.type) { return false; }
        if (lhs.start_token_.slice != rhs.start_token_.slice) { return false; }
        return lhs.is_equal(rhs);
    }

    template <LeafNode T> auto     is() const noexcept -> bool { return kind_ == T::KIND; }
    template <LeafNode... Ts> auto any() const noexcept -> bool {
        return ((kind_ == Ts::KIND) || ...);
    }

  protected:
    explicit Node(const Token& tok, NodeKind kind) noexcept : start_token_{tok}, kind_{kind} {}

    virtual auto is_equal(const Node& other) const noexcept -> bool = 0;

    // A safe alternative to a raw static cast for nodes.
    template <LeafNode T> static auto as(const Node& n) -> const T& {
        assert(n.is<T>());
        return static_cast<const T&>(n);
    }

    // Transfers ownership and downcasts a boxed node into the requested type.
    template <LeafNode To, NodeSubtype From> static auto downcast(Box<From>&& from) -> Box<To> {
        assert(from && from->template is<To>());
        return box_into<To>(std::move(from));
    }

  protected:
    const Token    start_token_;
    const NodeKind kind_;
};

template <typename Derived, typename Base> class NodeBase : public Base {
  protected:
    explicit NodeBase(const Token& tok) noexcept : Base{tok, Derived::KIND} {}
};

class Expression : public Node {
  protected:
    using Node::Node;

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

template <typename Derived> class ExprBase : public NodeBase<Derived, Expression> {
  protected:
    using NodeBase<Derived, Expression>::NodeBase;
};

class Statement : public Node {
  protected:
    using Node::Node;

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

template <typename Derived> class StmtBase : public NodeBase<Derived, Statement> {
  protected:
    using NodeBase<Derived, Statement>::NodeBase;
};

} // namespace conch::ast

template <conch::ast::LeafNode N> struct std::formatter<N> : std::formatter<std::string> {
    static constexpr auto parse(std::format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> auto format(const N& n, F& ctx) const {
        return std::formatter<std::string>::format(
            std::format("{}: {}", conch::enum_name(n.get_kind()), n.get_token()), ctx);
    }
};
