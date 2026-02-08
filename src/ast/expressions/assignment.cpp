#include "ast/expressions/assignment.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto AssignmentExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto AssignmentExpression::parse(Parser& parser, Box<Expression> assignee)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser, assignee);
}

} // namespace conch::ast
