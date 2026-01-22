#include "ast/expressions/if.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
