#include "ast/statements/expression.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ExpressionStatement::parse(Parser& parser)
    -> Expected<Box<ExpressionStatement>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    auto       expr        = TRY(parser.parse_expression());

    return make_box<ExpressionStatement>(start_token, std::move(expr));
}

} // namespace conch::ast
