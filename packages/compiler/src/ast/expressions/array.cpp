#include <algorithm>
#include <utility>

#include "ast/expressions/array.hpp"

#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

ArrayExpression::ArrayExpression(const Token&                          start_token,
                                 Optional<Box<USizeIntegerExpression>> size,
                                 std::vector<Box<Expression>>          items) noexcept
    : ExprBase{start_token}, size_{std::move(size)}, items_{std::move(items)} {}

ArrayExpression::~ArrayExpression() = default;

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ArrayExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    parser.advance();

    Optional<Box<USizeIntegerExpression>> size;
    if (!parser.current_token_is(TokenType::UNDERSCORE)) {
        if (parser.current_token_is(TokenType::RBRACKET)) {
            return make_parser_unexpected(ParserError::MISSING_ARRAY_SIZE_TOKEN, start_token);
        }

        // Verify the current token is valid before actually parsing it
        const auto integer_token = parser.current_token();
        if (!token_type::is_usize_int(integer_token.type)) {
            return make_parser_unexpected(ParserError::UNEXPECTED_ARRAY_SIZE_TOKEN, integer_token);
        }

        size.emplace(downcast<USizeIntegerExpression>(TRY(USizeIntegerExpression::parse(parser))));
    }

    TRY(parser.expect_peek(TokenType::RBRACKET));
    TRY(parser.expect_peek(TokenType::LBRACE));

    std::vector<Box<Expression>> items;
    size.and_then([&items](const auto& sz) -> decltype(size) {
        items.resize(sz->get_value());
        return nullopt;
    });

    // Current token is either the LBRACE at the start or a comma before parsing
    while (!parser.peek_token_is(TokenType::RBRACE) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        items.emplace_back(TRY(parser.parse_expression()));
        if (!parser.peek_token_is(TokenType::RBRACE)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }

    // Perform last minute bounds checks to reduce load on Sema
    TRY(parser.expect_peek(TokenType::RBRACE));
    if (size && items.size() != (*size)->get_value()) {
        return make_parser_unexpected(ParserError::INCORRECT_EXPLICIT_ARRAY_SIZE, start_token);
    }

    if (items.empty() || (size && (*size)->get_value() == 0)) {
        return make_parser_unexpected(ParserError::EMPTY_ARRAY, start_token);
    }
    return make_box<ArrayExpression>(start_token, std::move(size), std::move(items));
}

auto ArrayExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ArrayExpression>(other);
    return optional::unsafe_eq<USizeIntegerExpression>(size_, casted.size_) &&
           std::ranges::equal(
               items_, casted.items_, [](const auto& a, const auto& b) { return *a == *b; });
}

} // namespace conch::ast
