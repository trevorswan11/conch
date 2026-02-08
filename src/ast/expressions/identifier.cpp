#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(Parser& parser) // cppcheck-suppress constParameterReference
    -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (start_token.type != TokenType::IDENT && !start_token.is_primitive()) {
        return make_parser_unexpected(ParserError::ILLEGAL_IDENTIFIER, start_token);
    }

    return make_box<IdentifierExpression>(start_token, start_token.slice);
}

} // namespace conch::ast
