#include "ast/expressions/call.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(Parser& parser, Box<Expression> function)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    TRY(parser.expect_peek(TokenType::LPAREN));

    std::vector<Box<Expression>> arguments;
    while (!parser.peek_token_is(TokenType::RPAREN)) {
        auto argument = TRY(parser.parse_expression());
        arguments.emplace_back(std::move(argument));

        if (!parser.peek_token_is(TokenType::RPAREN)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }

    return make_box<CallExpression>(start_token, std::move(function), std::move(arguments));
}

} // namespace conch::ast
