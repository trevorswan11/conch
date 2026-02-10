#include "ast/expressions/dot.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto DotExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace conch::ast
