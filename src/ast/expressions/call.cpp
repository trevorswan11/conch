#include "ast/expressions/call.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
