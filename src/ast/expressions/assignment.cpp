#include "ast/expressions/assignment.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto AssignmentExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace conch::ast
