#pragma once

#include "lexer/token.hpp"

class Visitor;

namespace ast {

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = default;

    virtual auto accept(Visitor& v) -> void = 0;

  protected:
    Node(const Token& tok) noexcept : start_token_{tok} {}
    const Token start_token_;
};

class Expression : public Node {
  protected:
    Expression(const Token& tok) noexcept : Node{tok} {}
};

class Statement : public Node {
  protected:
    Statement(const Token& tok) noexcept : Node{tok} {}
};

} // namespace ast
