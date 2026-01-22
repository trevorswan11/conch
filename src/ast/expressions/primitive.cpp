#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
