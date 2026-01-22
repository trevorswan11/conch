#include "ast/statements/expression.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ExpressionStatement::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
