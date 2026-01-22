#pragma once

#include "lexer/token.hpp"

class Visitor;

namespace ast {

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = default;

    virtual auto accept(Visitor& v) const -> void = 0;

  protected:
    Node(const Token& tok) noexcept : start_token_{tok} {}
    const Token start_token_;
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
