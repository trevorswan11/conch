#include "ast/expressions/infix.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto InfixExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
