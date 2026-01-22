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

enum class ParserError {
    UNEXPECTED_TOKEN,
};

using AST              = std::vector<std::unique_ptr<ast::Node>>;
using ParserDiagnostic = Diagnostic<ParserError>;

class Parser {
  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto consume() -> std::pair<AST, std::span<const ParserDiagnostic>>;

  private:
    // Advances the parser n times, returning the resulting current token
    auto advance(uint8_t n = 1) noexcept -> const Token&;

    auto currentTokenIs(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peekTokenIs(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    [[nodiscard]] auto expectCurrent(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto currentError(TokenType expected) -> void { tokenMismatchError(expected, current_token_); }
    [[nodiscard]] auto expectPeek(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto peekError(TokenType expected) -> void { tokenMismatchError(expected, peek_token_); }

    auto currentPrecedence() const noexcept -> Precedence;
    auto peekPrecedence() const noexcept -> Precedence;

    auto tokenMismatchError(TokenType expected, const Token& actual) -> void;

    [[nodiscard]] auto parseStatement()
        -> Expected<std::unique_ptr<ast::Statement>, ParserDiagnostic>;

  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};

    std::vector<ParserDiagnostic> diagnostics_{};
};
