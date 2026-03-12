#include <algorithm>

#include "ast/expressions/union.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

UnionField::UnionField(Box<IdentifierExpression> ident, ExplicitType&& type) noexcept
    : ident_{std::move(ident)}, type_{std::move(type)} {}
UnionField::~UnionField() = default;

auto UnionField::is_equal(const UnionField& other) const noexcept -> bool {
    return *ident_ == *other.ident_ && type_ == other.type_;
}

UnionExpression::UnionExpression(const Token& start_token, std::vector<UnionField> fields) noexcept
    : ExprBase{start_token}, fields_{std::move(fields)} {}
UnionExpression::~UnionExpression() = default;

auto UnionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnionExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType::LBRACE));

    std::vector<UnionField> fields;
    while (!parser.peek_token_is(TokenType::RBRACE)) {
        TRY(parser.expect_peek(TokenType::IDENT));
        auto ident = downcast<IdentifierExpression>(TRY(IdentifierExpression::parse(parser)));

        TRY(parser.expect_peek(TokenType::COLON));
        auto type = TRY(ExplicitType::parse(parser));

        fields.emplace_back(std::move(ident), std::move(type));
        if (!parser.peek_token_is(TokenType::RBRACE)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }
    TRY(parser.expect_peek(TokenType::RBRACE));

    if (fields.empty()) { return make_parser_unexpected(ParserError::EMPTY_UNION, start_token); }
    return make_box<UnionExpression>(start_token, std::move(fields));
}

auto UnionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<UnionExpression>(other);
    return std::ranges::equal(fields_, casted.fields_);
}

} // namespace conch::ast
