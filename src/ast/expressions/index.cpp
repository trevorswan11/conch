#include "ast/expressions/index.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IndexExpression::parse(Parser& parser, Box<Expression> array)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (parser.current_token_is(TokenType::RBRACE)) {
        return make_parser_unexpected(ParserError::INDEX_MISSING_EXPRESSION, start_token);
    }
    parser.advance();

    auto idx_expr = TRY(parser.parse_expression());
    TRY(parser.expect_peek(TokenType::RBRACKET));
    return make_box<IndexExpression>(start_token, std::move(array), std::move(idx_expr));
}

} // namespace conch::ast
