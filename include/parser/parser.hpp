#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "core.hpp"

#include "parser/precedence.hpp"

#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

namespace ast {

class Node;
class Statement;

} // namespace ast

enum class ParserError : u8 {
    UNEXPECTED_TOKEN,
};

using AST              = std::vector<std::unique_ptr<ast::Node>>;
using ParserDiagnostic = Diagnostic<ParserError>;

class Parser {
  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto advance(uint8_t times = 1) noexcept -> const Token&;
    auto consume() -> std::pair<AST, std::span<const ParserDiagnostic>>;

    auto current_token_is(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peek_token_is(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    [[nodiscard]] auto expect_current(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto current_error(TokenType expected) -> void { tt_mismatch_error(expected, current_token_); }
    [[nodiscard]] auto expect_peek(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto peek_error(TokenType expected) -> void { tt_mismatch_error(expected, peek_token_); }

    auto current_precedence() const noexcept -> Precedence;
    auto peek_precedence() const noexcept -> Precedence;

  private:
    auto tt_mismatch_error(TokenType expected, const Token& actual) -> void;

    [[nodiscard]] auto parse_statement()
        -> Expected<std::unique_ptr<ast::Statement>, ParserDiagnostic>;

  private:
  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};

    std::vector<ParserDiagnostic> diagnostics_{};
};
