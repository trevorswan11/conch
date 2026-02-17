#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <utility>

#include "parser/parser.hpp"
#include "parser/precedence.hpp"

#include "lexer/keywords.hpp"
#include "lexer/token.hpp"

#include "ast/ast.hpp"

namespace conch {

auto Parser::reset(std::string_view input) noexcept -> void { *this = Parser{input}; }

auto Parser::advance(uint8_t times) noexcept -> const Token& {
    for (uint8_t i = 0; i < times; ++i) {
        if (current_token_.type == TokenType::END &&
            (input_.empty() && current_token_.location.is_at_start())) {
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

auto Parser::poll_current_precedence() const noexcept -> Precedence {
    return get_binding(current_token_.type)
        .transform([](const auto& binding) { return binding.second; })
        .value_or(Precedence::LOWEST);
}

auto Parser::poll_peek_precedence() const noexcept -> Precedence {
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

    const auto& prefix = poll_prefix_fn(current_token_.type);
    if (!prefix) {
        return make_parser_unexpected(
            std::format("No prefix parse function for {} found", enum_name(current_token_.type)),
            ParserError::MISSING_PREFIX_PARSER,
            current_token_);
    }
    auto lhs_expression = TRY((*prefix)(*this));

    while (!peek_token_is(TokenType::SEMICOLON) && precedence < poll_peek_precedence()) {
        const auto& infix = poll_infix_fn(peek_token_.type);
        if (!infix) { break; }

        if (advance().type == TokenType::END) {
            return make_parser_unexpected(ParserError::INFIX_MISSING_RHS, current_token_);
        }

        lhs_expression = TRY((*infix)(*this, std::move(lhs_expression)));
    }

    return lhs_expression;
}

[[nodiscard]] auto Parser::parse_restricted_statement(ParserError error)
    -> Expected<Box<ast::Statement>, ParserDiagnostic> {
    using namespace ast;
    auto clause = TRY(parse_statement());

    // The clause can only be a jump, block, or expression statement
    if (!clause->any<ExpressionStatement, JumpStatement, BlockStatement>()) {
        return make_parser_unexpected(error, clause->get_token());
    }
    return clause;
}

[[nodiscard]] auto Parser::try_parse_restricted_alternate(ParserError error)
    -> Expected<Optional<Box<ast::Statement>>, ParserDiagnostic> {
    if (peek_token_is(TokenType::ELSE)) {
        // Advance twice to actually look at the statement's first token
        advance(2);
        return TRY(parse_restricted_statement(error));
    }
    return nullopt;
}

using PrefixPair          = std::pair<TokenType, Parser::PrefixFn>;
constexpr auto PREFIX_FNS = []() {
    constexpr auto initial_prefixes = std::to_array<PrefixPair>({
        {TokenType::IDENT, ast::IdentifierExpression::parse},
        {TokenType::BYTE, ast::ByteExpression::parse},
        {TokenType::FLOAT, ast::FloatExpression::parse},
        {TokenType::BANG, ast::PrefixExpression::parse},
        {TokenType::NOT, ast::PrefixExpression::parse},
        {TokenType::MINUS, ast::PrefixExpression::parse},
        {TokenType::AND, ast::PrefixExpression::parse},
        {TokenType::AND_MUT, ast::PrefixExpression::parse},
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
        {TokenType::MATCH, ast::MatchExpression::parse},
        {TokenType::LBRACKET, ast::ArrayExpression::parse},
        {TokenType::FOR, ast::ForLoopExpression::parse},
        {TokenType::WHILE, ast::WhileLoopExpression::parse},
        {TokenType::DO, ast::DoWhileLoopExpression::parse},
        {TokenType::LOOP, ast::InfiniteLoopExpression::parse},
    });

    constexpr auto total_ints =
        static_cast<usize>(TokenType::UZINT_16) - static_cast<usize>(TokenType::INT_2) + 1;
    std::array<PrefixPair, total_ints> int_prefixes;
    usize                              cursor = 0;
    for (auto i = std::to_underlying(TokenType::INT_2);
         i <= std::to_underlying(TokenType::UZINT_16);
         i++, cursor++) {
        const auto tt = static_cast<TokenType>(i);
        using namespace token_type;
        switch (to_int_category(tt)) {
        case IntegerCategory::SIGNED_BASE:
            int_prefixes[cursor] = {tt, ast::SignedIntegerExpression::parse};
            break;
        case IntegerCategory::SIGNED_WIDE:
            int_prefixes[cursor] = {tt, ast::SignedLongIntegerExpression::parse};
            break;
        case IntegerCategory::SIGNED_SIZE:
            int_prefixes[cursor] = {tt, ast::ISizeIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_BASE:
            int_prefixes[cursor] = {tt, ast::UnsignedIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_WIDE:
            int_prefixes[cursor] = {tt, ast::UnsignedLongIntegerExpression::parse};
            break;
        case IntegerCategory::UNSIGNED_SIZE:
            int_prefixes[cursor] = {tt, ast::USizeIntegerExpression::parse};
            break;
        }
    }

    constexpr auto primitive_prefixes =
        ALL_PRIMITIVES | std::views::transform([](TokenType tt) -> PrefixPair {
            return {tt, ast::IdentifierExpression::parse};
        });

    constexpr auto materialized_primitives =
        materialize_sized_view<ALL_PRIMITIVES.size()>(primitive_prefixes);

    constexpr auto builtins_prefixes =
        ALL_BUILTINS | std::views::transform([](const auto& builtin) -> PrefixPair {
            return {builtin.second, ast::IdentifierExpression::parse};
        });

    constexpr auto materialized_builtins =
        materialize_sized_view<ALL_BUILTINS.size()>(builtins_prefixes);

    auto prefix_fns = concat_arrays(
        initial_prefixes, materialized_primitives, materialized_builtins, int_prefixes);
    std::ranges::sort(prefix_fns, {}, &PrefixPair::first);
    return prefix_fns;
}();

constexpr auto Parser::poll_prefix_fn(TokenType tt) noexcept -> Optional<const PrefixFn&> {
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

constexpr auto Parser::poll_infix_fn(TokenType tt) noexcept -> Optional<const InfixFn&> {
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
