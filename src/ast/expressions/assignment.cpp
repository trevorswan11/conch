#include "ast/expressions/assignment.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto AssignmentExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto AssignmentExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<AssignmentExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
