#pragma once

#include <string_view>
#include <vector>

#include "lexer/token.hpp"

class Lexer {
  public:
    Lexer() = default;
    explicit Lexer(std::string_view input) : input_{input} {}
    ~Lexer() = default;

    auto next() -> Token;
    auto consume() -> std::vector<Token>;

  private:
    auto skipWhitespace() -> void;
    auto luIdent(std::string_view ident) -> TokenType;

    auto readChar() -> void;
    auto readOperator() -> Token;
    auto readIdent() -> std::string_view;
    auto readNumber() -> Token;
    auto readString() -> Token;
    auto readMultilineString() -> Token;
    auto readCharacterLiteral() -> Token;
    auto readComment() -> Token;

  private:
    std::string_view input_;
    size_t           pos_{0};
    size_t           peek_pos_{0};
    char             current_byte_{0};

    size_t line_no_{1};
    size_t col_no_{0};
};
