#include "ast/expressions/index.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
