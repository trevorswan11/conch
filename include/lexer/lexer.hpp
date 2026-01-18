#pragma once

#include <string_view>
#include <vector>

#include "lexer/token.hpp"

class Lexer {
  public:
    Lexer() noexcept = default;
    explicit Lexer(std::string_view input) noexcept : input_{input} {}

    auto reset(std::string_view input = {}) noexcept -> void;
    auto next() -> Token;
    auto consume() -> std::vector<Token>;

  private:
    auto skipWhitespace() noexcept -> void;
    auto luIdent(std::string_view ident) noexcept -> TokenType;

    auto readChar() noexcept -> void;
    auto readOperator() -> Token;
    auto readIdent() -> std::string_view;
    auto readNumber() -> Token;
    auto readString() -> Token;
    auto readMultilineString() -> Token;
    auto readCharacterLiteral() -> Token;
    auto readComment() -> Token;

  private:
    std::string_view             input_{};
    size_t                       pos_{0};
    size_t                       peek_pos_{0};
    std::string_view::value_type current_byte_{0};

    size_t line_no_{1};
    size_t col_no_{0};
};
