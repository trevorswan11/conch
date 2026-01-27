#include <utility>

#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"

#include "core.hpp"
#include "parser/parser.hpp"
#include "visitor/visitor.hpp"

namespace ast {

Enumeration::Enumeration(std::unique_ptr<IdentifierExpression> enumeration) noexcept
    : Enumeration{std::move(enumeration), nullopt} {}

Enumeration::Enumeration(std::unique_ptr<IdentifierExpression> enumeration,
                         Optional<std::unique_ptr<Expression>> value) noexcept
    : enumeration_{std::move(enumeration)}, value_{std::move(value)} {}

Enumeration::~Enumeration() = default;

EnumExpression::EnumExpression(const Token&                          start_token,
                               std::unique_ptr<IdentifierExpression> name,
                               std::vector<Enumeration>              variants) noexcept
    : Expression{start_token}, name_{std::move(name)}, enumerations_{std::move(variants)} {}

EnumExpression::~EnumExpression() = default;

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto EnumExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<EnumExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType ::IDENT));
    auto type_name = TRY(IdentifierExpression::parse(parser));

    TRY(parser.expect_peek(TokenType ::LBRACE));
    if (parser.peek_token_is(TokenType::RBRACE)) {
        parser.advance();
        return make_parser_unexpected(ParserError::ENUM_MISSING_VARIANTS, std::move(start_token));
    }

    std::vector<Enumeration> variant;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        TRY(parser.expect_peek(TokenType ::IDENT));
        auto ident = TRY(IdentifierExpression::parse(parser));

        Optional<std::unique_ptr<Expression>> value = nullopt;
        if (parser.peek_token_is(TokenType::ASSIGN)) {
            parser.advance(2);
            value = TRY(parser.parse_expression());
        }
        variant.emplace_back(std::move(ident), std::move(value));

        // All variants require a trailing comma!
        auto peek = parser.expect_peek(TokenType::COMMA);
        if (!peek.has_value()) {
            return make_parser_unexpected(peek.error(), ParserError::MISSING_TRAILING_COMMA);
        }
    }

    TRY(parser.expect_peek(TokenType::RBRACE));
    return std::make_unique<EnumExpression>(start_token, std::move(type_name), std::move(variant));
}

} // namespace ast
