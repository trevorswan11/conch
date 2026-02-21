#include <algorithm>

#include "ast/expressions/array.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

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
    TRY(parser.expect_peek(TokenType::LBRACE));

    // Current token is either the LBRACE at the start or a comma before parsing
    std::vector<Box<Expression>> items;
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        items.emplace_back(TRY(parser.parse_expression()));
        if (!parser.peek_token_is(TokenType::RBRACE)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }

    // Perform last minute bounds checks to reduce load on Sema
    TRY(parser.expect_peek(TokenType::RBRACE));
    if (items.empty()) { return make_parser_unexpected(ParserError::EMPTY_ARRAY, start_token); }
    return make_box<ArrayExpression>(start_token, std::move(size), std::move(items));
}

auto ArrayExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ArrayExpression>(other);
    return optional::unsafe_eq<Expression>(size_, casted.size_) &&
           std::ranges::equal(
               items_, casted.items_, [](const auto& a, const auto& b) { return *a == *b; });
}

} // namespace conch::ast
