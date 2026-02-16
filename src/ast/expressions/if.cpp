#include "ast/expressions/if.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IfExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();

    // Conditions have to be surrounded by parentheses
    TRY(parser.expect_peek(TokenType::LPAREN));
    parser.advance();
    if (parser.current_token_is(TokenType::RPAREN)) {
        return make_parser_unexpected(ParserError::WHILE_MISSING_CONDITION, start_token);
    }

    auto condition = TRY(parser.parse_expression());
    TRY(parser.expect_peek(TokenType::RPAREN));

    // The consequence and alternate are trivially handled by restricted statement parsers
    parser.advance();
    auto consequence = TRY(parser.parse_restricted_statement(ParserError::ILLEGAL_IF_BRANCH));
    auto alternate   = TRY(parser.try_parse_restricted_alternate(ParserError::ILLEGAL_IF_BRANCH));

    return make_box<IfExpression>(
        start_token, std::move(condition), std::move(consequence), std::move(alternate));
}

} // namespace conch::ast
