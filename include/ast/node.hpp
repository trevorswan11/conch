#pragma once

#include "lexer/token.hpp"

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
    UNSIGNED_INTEGER_EXPRESSION,
    SIZE_INTEGER_EXPRESSION,
    BYTE_EXPRESSION,
    FLOAT_EXPRESSION,
    BOOL_EXPRESSION,
    VOID_EXPRESSION,
    NIL_EXPRESSION,
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
    NAMESPACE_STATEMENT,
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
    auto get_kind() const noexcept -> const Token& { return start_token_; }

    friend auto operator==(const Node& lhs, const Node& rhs) noexcept -> bool {
        if (lhs.kind_ != rhs.kind_) { return false; }
        if (lhs.start_token_.type != rhs.start_token_.type) { return false; }
        if (lhs.start_token_.slice != rhs.start_token_.slice) { return false; }

        return lhs.is_equal(rhs);
    }

  protected:
    explicit Node(const Token& tok, NodeKind kind) noexcept : start_token_{tok}, kind_{kind} {}

    virtual auto is_equal(const Node& other) const noexcept -> bool = 0;

    template <typename T> static const T& as(const Node& n) { return static_cast<const T&>(n); }

  protected:
    const Token    start_token_;
    const NodeKind kind_;
};

class Expression : public Node {
  protected:
    using Node::Node;

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

class Statement : public Node {
  protected:
    using Node::Node;

    virtual auto is_equal(const Node& other) const noexcept -> bool override = 0;
};

} // namespace conch::ast
