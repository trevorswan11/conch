#include <algorithm>
#include <format>

#include "parser/parser.hpp"
#include "parser/precedence.hpp"

#include "lexer/token.hpp"

#include "ast/node.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/decl.hpp"
#include "ast/statements/discard.hpp"
#include "ast/statements/expression.hpp"
#include "ast/statements/import.hpp"
#include "ast/statements/jump.hpp"

#include "ast/expressions/array.hpp"
#include "ast/expressions/assignment.hpp"
#include "ast/expressions/binary.hpp"
#include "ast/expressions/call.hpp"
#include "ast/expressions/do_while.hpp"
#include "ast/expressions/for.hpp"
#include "ast/expressions/function.hpp"
#include "ast/expressions/group.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/if.hpp"
#include "ast/expressions/index.hpp"
#include "ast/expressions/infinite_loop.hpp"
#include "ast/expressions/match.hpp"
#include "ast/expressions/prefix.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/expressions/range.hpp"
#include "ast/expressions/scope_resolve.hpp"
#include "ast/expressions/struct.hpp"
#include "ast/expressions/while.hpp"

namespace conch {

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::advance(uint8_t times) noexcept -> const Token& {
    for (uint8_t i = 0; i < times; ++i) {
        if (current_token_.type == TokenType::END) { break; }
        current_token_ = peek_token_;
        peek_token_    = lexer_.advance();
    }
    return current_token_;
}

auto Parser::consume() -> std::pair<ast::AST, std::span<const ParserDiagnostic>> {
    reset(input_);
    ast::AST ast;

    while (!current_token_is(TokenType::END)) {
        if (!current_token_is(TokenType::COMMENT)) {
            auto stmt = parse_statement();
            if (stmt) {
                ast.emplace_back(std::move(*stmt));
            } else {
                diagnostics_.emplace_back(std::move(stmt.error()));
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

auto Parser::parse_statement() -> Expected<Box<ast::Statement>, ParserDiagnostic> {
    switch (current_token_.type) {
    case TokenType::VAR:
    case TokenType::CONST:
    case TokenType::PRIVATE:
    case TokenType::STATIC:
    case TokenType::EXTERN:
    case TokenType::EXPORT: return ast::DeclStatement::parse(*this);
    case TokenType::BREAK:
    case TokenType::RETURN:
    case TokenType::CONTINUE: return ast::JumpStatement::parse(*this);
    case TokenType::IMPORT: return ast::ImportStatement::parse(*this);
    case TokenType::LBRACE: return ast::BlockStatement::parse(*this);
    case TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    default: return ast::ExpressionStatement::parse(*this);
    }

    std::unreachable();
}

auto Parser::parse_expression(Precedence precedence)
    -> Expected<Box<ast::Expression>, ParserDiagnostic> {
    if (current_token_is(TokenType::END)) {
        return make_parser_unexpected(ParserError::END_OF_TOKEN_STREAM, current_token_);
    }

    const auto& prefix = poll_prefix(current_token_.type);
    if (!prefix) {
        return make_parser_unexpected(
            std::format("No prefix parse function for {} found", enum_name(current_token_.type)),
            ParserError::MISSING_PREFIX_PARSER,
            current_token_);
    }
    auto lhs_expression = TRY((*prefix)(*this));

    while (!peek_token_is(TokenType::SEMICOLON) && precedence < peek_precedence()) {
        const auto& infix = poll_infix(peek_token_.type);
        if (!infix) { break; }

        if (advance().type == TokenType::END) {
            return make_parser_unexpected(ParserError::INFIX_MISSING_RHS, current_token_);
        }

        lhs_expression = TRY((*infix)(*this, std::move(lhs_expression)));
    }

    return lhs_expression;
}

auto Parser::poll_prefix(TokenType tt) noexcept -> Optional<const PrefixFn&> {
    const auto it = std::ranges::find(PREFIX_FNS, tt, &PrefixPair::first);
    return it == PREFIX_FNS.end() ? nullopt : Optional<const PrefixFn&>{it->second};
}

auto Parser::poll_infix(TokenType tt) noexcept -> Optional<const InfixFn&> {
    const auto it = std::ranges::find(INFIX_FNS, tt, &InfixPair::first);
    return it == INFIX_FNS.end() ? nullopt : Optional<const InfixFn&>{it->second};
}

auto Parser::tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic {
    return ParserDiagnostic{
        std::format("Expected token {}, found {}", enum_name(expected), enum_name(actual.type)),
        ParserError::UNEXPECTED_TOKEN,
        actual};
}

std::array<Parser::PrefixPair, 34> Parser::PREFIX_FNS = {
    PrefixPair{TokenType::IDENT, ast::IdentifierExpression::parse},
    PrefixPair{TokenType::INT_2, ast::IntegerExpression::parse},
    PrefixPair{TokenType::INT_8, ast::IntegerExpression::parse},
    PrefixPair{TokenType::INT_10, ast::IntegerExpression::parse},
    PrefixPair{TokenType::INT_16, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UINT_2, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UINT_8, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UINT_10, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UINT_16, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UZINT_2, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UZINT_8, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UZINT_10, ast::IntegerExpression::parse},
    PrefixPair{TokenType::UZINT_16, ast::IntegerExpression::parse},
    PrefixPair{TokenType::CHARACTER, ast::ByteExpression::parse},
    PrefixPair{TokenType::FLOAT, ast::FloatExpression::parse},
    PrefixPair{TokenType::BANG, ast::PrefixExpression::parse},
    PrefixPair{TokenType::NOT, ast::PrefixExpression::parse},
    PrefixPair{TokenType::MINUS, ast::PrefixExpression::parse},
    PrefixPair{TokenType::TRUE, ast::BoolExpression::parse},
    PrefixPair{TokenType::FALSE, ast::BoolExpression::parse},
    PrefixPair{TokenType::STRING, ast::StringExpression::parse},
    PrefixPair{TokenType::MULTILINE_STRING, ast::StringExpression::parse},
    PrefixPair{TokenType::LPAREN, ast::GroupedExpression::parse},
    PrefixPair{TokenType::IF, ast::IfExpression::parse},
    PrefixPair{TokenType::FUNCTION, ast::FunctionExpression::parse},
    PrefixPair{TokenType::STRUCT, ast::StructExpression::parse},
    PrefixPair{TokenType::ENUM, ast::FunctionExpression::parse},
    PrefixPair{TokenType::NIL, ast::NilExpression::parse},
    PrefixPair{TokenType::MATCH, ast::MatchExpression::parse},
    PrefixPair{TokenType::LBRACKET, ast::ArrayExpression::parse},
    PrefixPair{TokenType::FOR, ast::ForLoopExpression::parse},
    PrefixPair{TokenType::WHILE, ast::WhileLoopExpression::parse},
    PrefixPair{TokenType::DO, ast::DoWhileLoopExpression::parse},
    PrefixPair{TokenType::LOOP, ast::InfiniteLoopExpression::parse},
};

std::array<std::pair<TokenType, Parser::InfixFn>, 39> Parser::INFIX_FNS = {
    InfixPair{TokenType::PLUS, ast::BinaryExpression::parse},
    InfixPair{TokenType::MINUS, ast::BinaryExpression::parse},
    InfixPair{TokenType::STAR, ast::BinaryExpression::parse},
    InfixPair{TokenType::SLASH, ast::BinaryExpression::parse},
    InfixPair{TokenType::PERCENT, ast::BinaryExpression::parse},
    InfixPair{TokenType::STAR_STAR, ast::BinaryExpression::parse},
    InfixPair{TokenType::LT, ast::BinaryExpression::parse},
    InfixPair{TokenType::LTEQ, ast::BinaryExpression::parse},
    InfixPair{TokenType::GT, ast::BinaryExpression::parse},
    InfixPair{TokenType::GTEQ, ast::BinaryExpression::parse},
    InfixPair{TokenType::EQ, ast::BinaryExpression::parse},
    InfixPair{TokenType::NEQ, ast::BinaryExpression::parse},
    InfixPair{TokenType::BOOLEAN_AND, ast::BinaryExpression::parse},
    InfixPair{TokenType::BOOLEAN_OR, ast::BinaryExpression::parse},
    InfixPair{TokenType::AND, ast::BinaryExpression::parse},
    InfixPair{TokenType::OR, ast::BinaryExpression::parse},
    InfixPair{TokenType::XOR, ast::BinaryExpression::parse},
    InfixPair{TokenType::SHR, ast::BinaryExpression::parse},
    InfixPair{TokenType::SHL, ast::BinaryExpression::parse},
    InfixPair{TokenType::IS, ast::BinaryExpression::parse},
    InfixPair{TokenType::IN, ast::BinaryExpression::parse},
    InfixPair{TokenType::DOT_DOT, ast::RangeExpression::parse},
    InfixPair{TokenType::DOT_DOT_EQ, ast::RangeExpression::parse},
    InfixPair{TokenType::ORELSE, ast::BinaryExpression::parse},
    InfixPair{TokenType::LPAREN, ast::CallExpression::parse},
    InfixPair{TokenType::LBRACKET, ast::IndexExpression::parse},
    InfixPair{TokenType::ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::PLUS_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::MINUS_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::STAR_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::SLASH_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::PERCENT_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::AND_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::OR_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::SHL_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::SHR_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::NOT_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::XOR_ASSIGN, ast::AssignmentExpression::parse},
    InfixPair{TokenType::COLON_COLON, ast::ScopeResolutionExpression::parse},
};

} // namespace conch
