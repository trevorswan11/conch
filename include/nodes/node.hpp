#pragma once

#include <string>

#include "lexer/token.hpp"

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = 0;

    virtual auto stringify() const -> std::string = 0;
    // virtual auto analyze() -> void                            = 0;

  protected:
    Node(const Token& tok) : start_token_{tok} {}
    Token start_token_;
};

class Expression : public Node {};

class Statement : public Node {};
