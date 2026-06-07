#pragma once

#include <iterator>
#include <string_view>
#include <vector>

#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include <option.hh>
#include <types.hh>

namespace ghoti::syntax {

class Lexer {
  public:
    class Iterator {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type        = Token;
        using difference_type   = idiff;
        using pointer           = const Token*;
        using reference         = const Token&;

      public:
        constexpr Iterator(Lexer& lexer, const Token& current_token)
            : lexer_{lexer}, current_token_{current_token} {}

        constexpr auto operator++() -> Iterator& {
            current_token_ = lexer_.advance();
            return *this;
        }

        constexpr auto operator*() const noexcept -> reference { return current_token_; }
        constexpr auto operator->() const noexcept -> pointer { return &current_token_; }

        [[nodiscard]] constexpr auto operator==(std::default_sentinel_t) const noexcept -> bool {
            return current_token_.type == TokenType::END;
        }

      private:
        Lexer& lexer_;
        Token  current_token_;
    };

    class Snapshot {
      public:
        constexpr explicit Snapshot(const Lexer& l) noexcept
            : pos_{l.pos_}, peek_pos_{l.peek_pos_}, current_byte_{l.current_byte_},
              line_no_{l.line_no_}, col_no_{l.col_no_} {}

      private:
        usize pos_;
        usize peek_pos_;
        byte  current_byte_;
        usize line_no_;
        usize col_no_;

        friend class Lexer;
    };

  public:
    Lexer() noexcept = default;
    explicit Lexer(std::string_view input) noexcept : input_{input} {
        // `read_character` advances the column but it isn't consuming here so we reset it
        read_character();
        col_no_ = 0;
    }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance() noexcept -> Token;
    auto consume() -> std::vector<Token>;

    auto        begin() noexcept -> Iterator { return Iterator{*this, advance()}; }
    static auto end() noexcept -> std::default_sentinel_t { return std::default_sentinel; }

  private:
    auto        skip_whitespace() noexcept -> void;
    static auto lu_builtin(std::string_view ident) noexcept -> TokenType;
    static auto lu_ident(std::string_view ident) noexcept -> TokenType;

    // Reads n characters from the input stream
    auto               read_character(u8 n = 1) noexcept -> void;
    [[nodiscard]] auto read_operator() const noexcept -> opt::Option<Token>;
    auto               read_ident(bool builtin) noexcept -> std::string_view;
    auto               read_number() noexcept -> Token;
    auto               read_escape() noexcept -> byte;
    auto               read_string() noexcept -> Token;
    auto               read_multiline_string() noexcept -> Token;
    auto               read_byte_literal() noexcept -> Token;
    auto               read_comment() noexcept -> Token;

    // Sets the lexer to the snapshot, very cheap operation.
    constexpr auto restore(const Snapshot& state) noexcept -> void {
        pos_          = state.pos_;
        peek_pos_     = state.peek_pos_;
        current_byte_ = state.current_byte_;
        line_no_      = state.line_no_;
        col_no_       = state.col_no_;
    }

  private:
    std::string_view input_;
    usize            pos_{0};
    usize            peek_pos_{0};
    byte             current_byte_{0};

    usize line_no_{0};
    usize col_no_{0};

    friend class Parser;
};

} // namespace ghoti::syntax
