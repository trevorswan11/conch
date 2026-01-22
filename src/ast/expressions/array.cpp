#include "ast/expressions/array.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
