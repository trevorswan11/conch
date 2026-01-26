#include <format>

#include "parser/parser.hpp"
#include "parser/precedence.hpp"

#include "lexer/token.hpp"

#include "ast/node.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/decl.hpp"
#include "ast/statements/discard.hpp"
#include "ast/statements/expression.hpp"
#include "ast/statements/impl.hpp"
#include "ast/statements/import.hpp"
#include "ast/statements/jump.hpp"

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::advance(uint8_t times) noexcept -> const Token& {
    for (uint8_t i = 0; i < times; ++i) {
        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

auto Parser::consume() -> std::pair<AST, std::span<const ParserDiagnostic>> {
    reset(input_);
    AST ast;

    while (!current_token_is(TokenType::END)) {
        if (!current_token_is(TokenType::COMMENT)) {
            auto stmt = parse_statement();
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

auto Parser::expect_current(TokenType expected) -> Expected<std::monostate, ParserDiagnostic> {
    if (current_token_is(expected)) {
        advance();
        return {};
    }
    return Unexpected{current_error(expected)};
}

auto Parser::expect_peek(TokenType expected) -> Expected<std::monostate, ParserDiagnostic> {
    if (peek_token_is(expected)) {
        advance();
        return {};
    }
    return Unexpected{peek_error(expected)};
}

auto Parser::current_precedence() const noexcept -> Precedence {
    return get_binding(current_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto Parser::peek_precedence() const noexcept -> Precedence {
    return get_binding(peek_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto Parser::tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic {
    return ParserDiagnostic{
        std::format("Expected token {}, found {}", enum_name(expected), enum_name(actual.type)),
        ParserError::UNEXPECTED_TOKEN,
        actual.line,
        actual.column};
}

[[nodiscard]] auto Parser::parse_statement()
    -> Expected<std::unique_ptr<ast::Statement>, ParserDiagnostic> {
    using namespace ast;
    switch (current_token_.type) {
    case TokenType::VAR:
    case TokenType::CONST: return DeclStatement::parse(*this);
    case TokenType::BREAK:
    case TokenType::RETURN:
    case TokenType::CONTINUE: return JumpStatement::parse(*this);
    case TokenType::IMPL: return ImplStatement::parse(*this);
    case TokenType::IMPORT: return ImportStatement::parse(*this);
    case TokenType::LBRACE: return BlockStatement::parse(*this);
    case TokenType::UNDERSCORE: return DiscardStatement::parse(*this);
    default: return ExpressionStatement::parse(*this);
    }

    std::unreachable();
}

[[nodiscard]] auto parse_expression(Precedence precedence)
    -> Expected<std::unique_ptr<ast::Expression>, ParserDiagnostic> {
    TODO(precedence);
}
