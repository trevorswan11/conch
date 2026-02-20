#include "ast/expressions/identifier.hpp"

#include "ast/visitor.hpp"

namespace conch::ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(Parser& parser) // cppcheck-suppress constParameterReference
    -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (!start_token.is_valid_ident()) {
        return make_parser_unexpected(ParserError::ILLEGAL_IDENTIFIER, start_token);
    }

    return make_box<IdentifierExpression>(start_token);
}

} // namespace conch::ast
