#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(const Parser& parser)
    -> Expected<std::unique_ptr<IdentifierExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    if (start_token.type != TokenType::IDENT && !start_token.primitive()) {
        return make_parser_unexpected(ParserError::ILLEGAL_IDENTIFIER, start_token);
    }

    return std::make_unique<IdentifierExpression>(start_token, std::string{start_token.slice});
}

} // namespace ast
