#include <algorithm>

#include "ast/expressions/array.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

ArrayExpression::ArrayExpression(const Token&                 start_token,
                                 Optional<Box<Expression>>    size,
                                 ExplicitType&&               item_type,
                                 std::vector<Box<Expression>> items) noexcept
    : ExprBase{start_token}, size_{std::move(size)}, item_type_{std::move(item_type)},
      items_{std::move(items)} {}
ArrayExpression::~ArrayExpression() = default;

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ArrayExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    parser.advance();

    Optional<Box<Expression>> size;
    if (!parser.current_token_is(TokenType::UNDERSCORE)) {
        if (parser.current_token_is(TokenType::RBRACKET)) {
            return make_parser_unexpected(ParserError::MISSING_ARRAY_SIZE_TOKEN, start_token);
        }
        size.emplace(TRY(parser.parse_expression()));
    }

    TRY(parser.expect_peek(TokenType::RBRACKET));
    auto item_type = TRY(ExplicitType::parse(parser));

    TRY(parser.expect_peek(TokenType::LBRACE));

    // Current token is either the LBRACE at the start or a comma before parsing
    std::vector<Box<Expression>> items;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        items.emplace_back(TRY(parser.parse_expression()));
        if (!parser.peek_token_is(TokenType::RBRACE)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }

    // Perform last minute ident/size checks to reduce load on Sema
    TRY(parser.expect_peek(TokenType::RBRACE));
    if (size) {
        const auto& size_expr = *(*size);
        if (size_expr.is<USizeIntegerExpression>()) {
            const auto& explicit_size = as<USizeIntegerExpression>(size_expr);
            const auto& size_token    = size_expr.get_token();

            // Enforce full initialization
            if (items.size() != explicit_size.get_value()) {
                return make_parser_unexpected(ParserError::EXPLICIT_ARRAY_SIZE_MISMATCH,
                                              size_token);
            }
        } else if (!size_expr.is<IdentifierExpression>()) {
            return make_parser_unexpected(ParserError::ILLEGAL_ARRAY_SIZE_TYPE,
                                          size_expr.get_token());
        }
    }

    return make_box<ArrayExpression>(
        start_token, std::move(size), std::move(item_type), std::move(items));
}

auto ArrayExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ArrayExpression>(other);
    return optional::unsafe_eq<Expression>(size_, casted.size_) &&
           item_type_ == casted.item_type_ &&
           std::ranges::equal(
               items_, casted.items_, [](const auto& a, const auto& b) { return *a == *b; });
}

} // namespace conch::ast
