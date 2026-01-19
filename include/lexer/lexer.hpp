#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include "lexer/token.hpp"

#include "core.hpp"

class Lexer {
  public:
    Lexer() noexcept = default;
    explicit Lexer(std::string_view input) noexcept : input_{input} { readInputCharacter(); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance() noexcept -> Token;
    auto consume() -> std::vector<Token>;

  private:
    auto        skipWhitespace() noexcept -> void;
    static auto luIdent(std::string_view ident) noexcept -> TokenType;

    auto readInputCharacter(uint8_t repeats = 0) noexcept -> void;
    auto readOperator() const noexcept -> std::optional<Token>;
    auto readIdent() noexcept -> std::string_view;
    auto readNumber() noexcept -> Token;
    auto readEscape() noexcept -> byte;
    auto readString() noexcept -> Token;
    auto readMultilineString() noexcept -> Token;
    auto readByteLiteral() noexcept -> Token;
    auto readComment() noexcept -> Token;

  private:
    std::string_view input_{};
    size_t           pos_{0};
    size_t           peek_pos_{0};
    byte             current_byte_{0};

    size_t line_no_{1};
    size_t col_no_{0};
};
