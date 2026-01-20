#pragma once

#include <memory>
#include <string_view>
#include <variant>
#include <vector>

#include "core.hpp"

#include "parser/precedence.hpp"

#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

class Node;
class Statement;

enum class ParserError {
    UNEXPECTED_TOKEN,
};

using AST = int;

class Parser {
  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept : input_{input}, lexer_{input} { advance(2); }

    auto reset(std::string_view input = {}) noexcept -> void;
    auto consume() -> std::variant<AST, std::vector<Diagnostic>>;

  private:
    // Advances the parser n times, returning the resulting current token
    auto advance(uint8_t n = 1) noexcept -> const Token&;

    auto currentTokenIs(TokenType t) const noexcept -> bool { return current_token_.type == t; }
    auto peekTokenIs(TokenType t) const noexcept -> bool { return peek_token_.type == t; }

    [[nodiscard]] auto expectCurrent(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto currentError(TokenType expected) -> void { pushDiagnostic(expected, current_token_); }
    [[nodiscard]] auto expectPeek(TokenType expected) -> Expected<std::monostate, ParserError>;
    auto peekError(TokenType expected) -> void { pushDiagnostic(expected, peek_token_); }

    auto currentPrecedence() const noexcept -> Precedence;
    auto peekPrecedence() const noexcept -> Precedence;

    auto pushDiagnostic(TokenType expected, const Token& actual) -> void;

    [[nodiscard]] auto parseStatement() -> std::unique_ptr<Statement>;

  private:
    std::string_view input_;
    Lexer            lexer_{};
    Token            current_token_{};
    Token            peek_token_{};

    std::vector<Diagnostic> diagnostics_{};
};
