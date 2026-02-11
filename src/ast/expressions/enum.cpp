#include <algorithm>
#include <utility>

#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

Enumeration::Enumeration(Box<IdentifierExpression> enumeration,
                         Optional<Box<Expression>> value) noexcept
    : enumeration_{std::move(enumeration)}, value_{std::move(value)} {}

Enumeration::~Enumeration() = default;

EnumExpression::EnumExpression(const Token&                        start_token,
                               Optional<Box<IdentifierExpression>> underlying,
                               std::vector<Enumeration>            enumerations) noexcept
    : ExprBase{start_token}, underlying_{std::move(underlying)},
      enumerations_{std::move(enumerations)} {}

EnumExpression::~EnumExpression() = default;

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto EnumExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    Box<IdentifierExpression> underlying;
    if (parser.peek_token_is(TokenType::COLON)) {
        parser.advance();
        TRY(parser.expect_peek(TokenType ::IDENT));
        underlying = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));
    }

    TRY(parser.expect_peek(TokenType ::LBRACE));
    if (parser.peek_token_is(TokenType::RBRACE)) {
        parser.advance();
        return make_parser_unexpected(ParserError::ENUM_MISSING_VARIANTS, std::move(start_token));
    }

    std::vector<Enumeration> enumeration;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        TRY(parser.expect_peek(TokenType ::IDENT));
        auto ident = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

        Optional<Box<Expression>> value = nullopt;
        if (parser.peek_token_is(TokenType::ASSIGN)) {
            parser.advance(2);
            value = TRY(parser.parse_expression());
        }
        enumeration.emplace_back(std::move(ident), std::move(value));

        // All variants require a trailing comma!
        auto peek = parser.expect_peek(TokenType::COMMA);
        if (!peek.has_value()) {
            return make_parser_unexpected(peek.error(), ParserError::MISSING_TRAILING_COMMA);
        }
    }

    TRY(parser.expect_peek(TokenType::RBRACE));
    return make_box<EnumExpression>(start_token, std::move(underlying), std::move(enumeration));
}

auto EnumExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<EnumExpression>(other);
    return optional::unsafe_eq<IdentifierExpression>(underlying_, casted.underlying_) &&
           std::ranges::equal(
               enumerations_, casted.enumerations_, [](const auto& a, const auto& b) {
                   return *a.enumeration_ == *b.enumeration_ &&
                          optional::unsafe_eq<Expression>(a.value_, b.value_);
               });
}

} // namespace conch::ast
