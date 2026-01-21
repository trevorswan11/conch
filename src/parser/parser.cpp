#include <format>

#include "parser/parser.hpp"
#include "parser/precedence.hpp"

#include "lexer/token.hpp"

#include "ast/node.hpp"

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::consume() -> std::pair<AST, std::span<const ParserDiagnostic>> {
    reset(input_);
    AST ast;

    while (!currentTokenIs(TokenType::END)) {
        if (!currentTokenIs(TokenType::COMMENT)) {
            auto stmt = parseStatement();
            if (stmt) {
                ast.emplace_back(std::move(*stmt));
            } else {
                diagnostics_.emplace_back(stmt.error());
            }
        }
        advance();
    }

    return {std::move(ast), diagnostics_};
}

auto Parser::tokenMismatchError(TokenType expected, const Token& actual) -> void {
    diagnostics_.emplace_back(
        std::format("Expected token {}, found {}", enum_name(expected), enum_name(actual.type)),
        ParserError::UNEXPECTED_TOKEN,
        actual.line,
        actual.column);
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

[[nodiscard]] auto Parser::parseStatement()
    -> Expected<std::unique_ptr<ast::Statement>, ParserDiagnostic> {
    return nullptr;
}
