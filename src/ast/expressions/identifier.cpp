#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IdentifierExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<IdentifierExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
