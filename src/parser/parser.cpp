#include <algorithm>
#include <array>
#include <format>
#include <ranges>

#include "parser/parser.hpp"
#include "parser/precedence.hpp"

#include "lexer/keywords.hpp"
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
        if (current_token_.type == TokenType::END &&
            (input_.empty() && current_token_.location.at_start())) {
            break;
        }

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
            if (peek_token_is(TokenType::SEMICOLON)) { advance(); }
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
    case TokenType::EXTERN:
    case TokenType::EXPORT:     return ast::DeclStatement::parse(*this);
    case TokenType::BREAK:
    case TokenType::RETURN:
    case TokenType::CONTINUE:   return ast::JumpStatement::parse(*this);
    case TokenType::IMPORT:     return ast::ImportStatement::parse(*this);
    case TokenType::LBRACE:     return ast::BlockStatement::parse(*this);
    case TokenType::UNDERSCORE: return ast::DiscardStatement::parse(*this);
    default:                    return ast::ExpressionStatement::parse(*this);
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

using PrefixPair          = std::pair<TokenType, Parser::PrefixFn>;
constexpr auto PREFIX_FNS = []() {
    constexpr auto initial_prefixes = std::to_array<PrefixPair>({
        {TokenType::IDENT, ast::IdentifierExpression::parse},
        {TokenType::INT_2, ast::SignedIntegerExpression::parse},
        {TokenType::INT_8, ast::SignedIntegerExpression::parse},
        {TokenType::INT_10, ast::SignedIntegerExpression::parse},
        {TokenType::INT_16, ast::SignedIntegerExpression::parse},
        {TokenType::UINT_2, ast::UnsignedIntegerExpression::parse},
        {TokenType::UINT_8, ast::UnsignedIntegerExpression::parse},
        {TokenType::UINT_10, ast::UnsignedIntegerExpression::parse},
        {TokenType::UINT_16, ast::UnsignedIntegerExpression::parse},
        {TokenType::UZINT_2, ast::SizeIntegerExpression::parse},
        {TokenType::UZINT_8, ast::SizeIntegerExpression::parse},
        {TokenType::UZINT_10, ast::SizeIntegerExpression::parse},
        {TokenType::UZINT_16, ast::SizeIntegerExpression::parse},
        {TokenType::BYTE, ast::ByteExpression::parse},
        {TokenType::FLOAT, ast::FloatExpression::parse},
        {TokenType::BANG, ast::PrefixExpression::parse},
        {TokenType::NOT, ast::PrefixExpression::parse},
        {TokenType::MINUS, ast::PrefixExpression::parse},
        {TokenType::TRUE, ast::BoolExpression::parse},
        {TokenType::FALSE, ast::BoolExpression::parse},
        {TokenType::STRING, ast::StringExpression::parse},
        {TokenType::MULTILINE_STRING, ast::StringExpression::parse},
        {TokenType::LPAREN, ast::GroupedExpression::parse},
        {TokenType::IF, ast::IfExpression::parse},
        {TokenType::FUNCTION, ast::FunctionExpression::parse},
        {TokenType::MUT, ast::FunctionExpression::parse},
        {TokenType::PACKED, ast::StructExpression::parse},
        {TokenType::STRUCT, ast::StructExpression::parse},
        {TokenType::ENUM, ast::FunctionExpression::parse},
        {TokenType::NIL, ast::NilExpression::parse},
        {TokenType::MATCH, ast::MatchExpression::parse},
        {TokenType::LBRACKET, ast::ArrayExpression::parse},
        {TokenType::FOR, ast::ForLoopExpression::parse},
        {TokenType::WHILE, ast::WhileLoopExpression::parse},
        {TokenType::DO, ast::DoWhileLoopExpression::parse},
        {TokenType::LOOP, ast::InfiniteLoopExpression::parse},
    });

    constexpr auto primitive_prefixes =
        ALL_PRIMITIVES | std::views::transform([](TokenType tt) -> PrefixPair {
            return {tt, ast::IdentifierExpression::parse};
        });

    constexpr auto materialized_keywords =
        materialize_sized_view<ALL_PRIMITIVES.size()>(primitive_prefixes);
    auto prefix_fns = concat_arrays(initial_prefixes, materialized_keywords);
    std::ranges::sort(prefix_fns, {}, &PrefixPair::first);
    return prefix_fns;
}();

constexpr auto Parser::poll_prefix(TokenType tt) noexcept -> Optional<const PrefixFn&> {
    const auto it = std::ranges::lower_bound(PREFIX_FNS, tt, {}, &PrefixPair::first);
    if (it == PREFIX_FNS.end() || it->first != tt) { return nullopt; }
    return Optional<const PrefixFn&>{it->second};
}

using InfixPair          = std::pair<TokenType, Parser::InfixFn>;
constexpr auto INFIX_FNS = []() {
    auto infix_fns = std::to_array<InfixPair>({
        {TokenType::PLUS, ast::BinaryExpression::parse},
        {TokenType::MINUS, ast::BinaryExpression::parse},
        {TokenType::STAR, ast::BinaryExpression::parse},
        {TokenType::SLASH, ast::BinaryExpression::parse},
        {TokenType::PERCENT, ast::BinaryExpression::parse},
        {TokenType::STAR_STAR, ast::BinaryExpression::parse},
        {TokenType::LT, ast::BinaryExpression::parse},
        {TokenType::LTEQ, ast::BinaryExpression::parse},
        {TokenType::GT, ast::BinaryExpression::parse},
        {TokenType::GTEQ, ast::BinaryExpression::parse},
        {TokenType::EQ, ast::BinaryExpression::parse},
        {TokenType::NEQ, ast::BinaryExpression::parse},
        {TokenType::BOOLEAN_AND, ast::BinaryExpression::parse},
        {TokenType::BOOLEAN_OR, ast::BinaryExpression::parse},
        {TokenType::AND, ast::BinaryExpression::parse},
        {TokenType::OR, ast::BinaryExpression::parse},
        {TokenType::XOR, ast::BinaryExpression::parse},
        {TokenType::SHR, ast::BinaryExpression::parse},
        {TokenType::SHL, ast::BinaryExpression::parse},
        {TokenType::IS, ast::BinaryExpression::parse},
        {TokenType::IN, ast::BinaryExpression::parse},
        {TokenType::DOT_DOT, ast::RangeExpression::parse},
        {TokenType::DOT_DOT_EQ, ast::RangeExpression::parse},
        {TokenType::ORELSE, ast::BinaryExpression::parse},
        {TokenType::LPAREN, ast::CallExpression::parse},
        {TokenType::LBRACKET, ast::IndexExpression::parse},
        {TokenType::ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::PLUS_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::MINUS_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::STAR_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::SLASH_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::PERCENT_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::AND_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::OR_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::SHL_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::SHR_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::NOT_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::XOR_ASSIGN, ast::AssignmentExpression::parse},
        {TokenType::COLON_COLON, ast::ScopeResolutionExpression::parse},
    });

    std::ranges::sort(infix_fns, {}, &InfixPair::first);
    return infix_fns;
}();

constexpr auto Parser::poll_infix(TokenType tt) noexcept -> Optional<const InfixFn&> {
    const auto it = std::ranges::lower_bound(INFIX_FNS, tt, {}, &InfixPair::first);
    if (it == INFIX_FNS.end() || it->first != tt) { return nullopt; }
    return Optional<const InfixFn&>{it->second};
}

auto Parser::tt_mismatch_error(TokenType expected, const Token& actual) -> ParserDiagnostic {
    return ParserDiagnostic{
        std::format("Expected token {}, found {}", enum_name(expected), enum_name(actual.type)),
        ParserError::UNEXPECTED_TOKEN,
        actual};
}

} // namespace conch
