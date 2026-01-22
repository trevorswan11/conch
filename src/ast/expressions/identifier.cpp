#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IdentifierExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
