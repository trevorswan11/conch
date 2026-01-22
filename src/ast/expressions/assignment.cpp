#include "ast/expressions/assignment.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto AssignmentExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
