#include "ast/statements/expression.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ExpressionStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<ExpressionStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
