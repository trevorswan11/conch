#pragma once

#include <cstdint>
#include <iterator>
#include <string_view>
#include <vector>

#include "lexer/token.hpp"

#include "util/common.hpp"
#include "util/optional.hpp"

namespace conch {

class Lexer {
  public:
    class Iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = Token;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const Token*;
        using reference         = const Token&;

      public:
        explicit Iterator(Lexer& lexer, const Token& current_token)
            : lexer_{lexer}, current_token_{current_token} {}

        auto operator++() -> Iterator& {
            current_token_ = lexer_.advance();
            return *this;
        }

        auto operator*() const noexcept -> reference { return current_token_; }
        auto operator->() const noexcept -> pointer { return &current_token_; }

        bool operator==(std::default_sentinel_t) const {
            return current_token_.type == TokenType::END;
        }

      private:
        Lexer& lexer_;
        Token  current_token_;
    };

  public:
    Lexer() noexcept = default;
    explicit Lexer(std::string_view input) noexcept : input_{input} { read_character(); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance() noexcept -> Token;
    auto consume() -> std::vector<Token>;

    auto begin() -> Iterator { return Iterator{*this, advance()}; }
    auto end() const noexcept -> std::default_sentinel_t { return std::default_sentinel; }

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
