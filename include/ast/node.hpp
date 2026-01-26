#pragma once

#include "lexer/token.hpp"

class Visitor;

namespace ast {

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = default;

    Node(const Node&)                = delete;
    Node& operator=(const Node&)     = delete;
    Node(Node&&) noexcept            = default;
    Node& operator=(Node&&) noexcept = default;

    virtual auto accept(Visitor& v) const -> void = 0;

    auto start_token() const noexcept -> const Token& { return start_token_; }

  protected:
    explicit Node(const Token& tok) noexcept : start_token_{tok} {}
    Token start_token_;
};

class Expression : public Node {
  protected:
    using Node::Node;
};

class Statement : public Node {
  protected:
    using Node::Node;
};

} // namespace ast
