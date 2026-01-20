#pragma once

#include <sstream>

#include "lexer/token.hpp"

class Node {
  public:
    Node()          = delete;
    virtual ~Node() = 0;

    virtual auto reconstruct(std::ostringstream& oss) -> void = 0;
    // virtual auto analyze() -> void                            = 0;

  protected:
    Node(const Token& tok) : start_token_{tok} {}
    Token start_token_;
};
