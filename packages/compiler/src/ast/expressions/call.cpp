#include "ast/expressions/call.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(Parser& parser, Box<Expression> function)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    std::vector<Box<Expression>> arguments;
    while (!parser.peek_token_is(TokenType::RPAREN) && !parser.peek_token_is(TokenType::END)) {
        parser.advance();
        auto argument = TRY(parser.parse_expression());
        arguments.emplace_back(std::move(argument));

        if (!parser.peek_token_is(TokenType::RPAREN)) { TRY(parser.expect_peek(TokenType::COMMA)); }
    }
    TRY(parser.expect_peek(TokenType::RPAREN));

    return make_box<CallExpression>(
        function->get_token(), std::move(function), std::move(arguments));
}

} // namespace conch::ast
