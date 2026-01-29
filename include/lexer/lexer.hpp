#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "lexer/token.hpp"

#include "util/common.hpp"
#include "util/optional.hpp"

namespace conch {

class Lexer {
  public:
    Lexer() noexcept = default;
    explicit Lexer(std::string_view input) noexcept : input_{input} { read_character(); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance() noexcept -> Token;
    auto consume() -> std::vector<Token>;

  private:
    auto        skip_whitespace() noexcept -> void;
    static auto lu_ident(std::string_view ident) noexcept -> TokenType;

    auto read_character(uint8_t n = 1) noexcept -> void;
    auto read_operator() const noexcept -> Optional<Token>;
    auto read_ident() noexcept -> std::string_view;
    auto read_number() noexcept -> Token;
    auto read_escape() noexcept -> byte;
    auto read_string() noexcept -> Token;
    auto read_multiline_string() noexcept -> Token;
    auto read_byte_literal() noexcept -> Token;
    auto read_comment() noexcept -> Token;

  private:
    std::string_view input_{};
    usize            pos_{0};
    usize            peek_pos_{0};
    byte             current_byte_{0};

    usize line_no_{1};
    usize col_no_{0};
};

} // namespace conch
