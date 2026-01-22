#include "ast/expressions/prefix.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto PrefixExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
