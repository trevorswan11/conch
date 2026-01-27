#include "ast/statements/expression.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ExpressionStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<ExpressionStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    auto       expr        = TRY(parser.parse_expression());

    return std::make_unique<ExpressionStatement>(start_token, std::move(expr));
}

} // namespace ast
