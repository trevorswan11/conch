#include "ast/expressions/range.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto RangeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace conch::ast
