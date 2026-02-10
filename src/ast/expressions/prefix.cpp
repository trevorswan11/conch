#include "ast/expressions/prefix.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto PrefixExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto PrefixExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto prefix_token = parser.current_token();
    if (parser.peek_token_is(TokenType::END)) {
        return make_parser_unexpected(ParserError::PREFIX_MISSING_OPERAND, prefix_token);
    }
    parser.advance();

    auto operand = TRY(parser.parse_expression(Precedence::PREFIX));
    return make_box<PrefixExpression>(prefix_token, std::move(operand));
}

} // namespace conch::ast
