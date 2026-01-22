#include "ast/expressions/enum.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto EnumExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
