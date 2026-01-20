#include <expected>
#include <format>

#include "parser/parser.hpp"

#include "lexer/token.hpp"
#include "nodes/statements/statement.hpp"
#include "parser/precedence.hpp"

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::consume() -> std::variant<std::vector<std::unique_ptr<Node>>> {
    reset(input_);
    std::vector<std::unique_ptr<Node>> ast;
    
    while (!currentTokenIs(TokenType::END)) {
        if (!currentTokenIs(TokenType::COMMENT)) {
            
        }
        advance();
    }
}

auto Parser::pushDiagnostic(TokenType expected, const Token& actual) -> void {
    diagnostics_.emplace_back(Diagnostic{
        .message =
            std::format("Expected token {}, found {}", enum_name(expected), enum_name(actual.type)),
        .line   = actual.line,
        .column = actual.column,
    });
}

auto Parser::expectCurrent(TokenType expected) -> Expected<std::monostate, ParserError> {
    if (currentTokenIs(expected)) {
        advance();
        return {};
    }

    currentError(expected);
    return Unexpected{ParserError::UNEXPECTED_TOKEN};
}

auto Parser::expectPeek(TokenType expected) -> Expected<std::monostate, ParserError> {
    if (peekTokenIs(expected)) {
        advance();
        return {};
    }

    peekError(expected);
    return Unexpected{ParserError::UNEXPECTED_TOKEN};
}

auto Parser::currentPrecedence() const noexcept -> Precedence {
    return get_binding(current_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto Parser::peekPrecedence() const noexcept -> Precedence {
    return get_binding(peek_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto Parser::advance(uint8_t n) noexcept -> const Token& {
    for (uint8_t i = 0; i < n; ++i) {
        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

[[nodiscard]] auto Parser::parseStatement() -> std::unique_ptr<Statement> {
    return nullptr;
}
