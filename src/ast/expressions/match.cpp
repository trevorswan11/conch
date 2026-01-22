#include "ast/expressions/match.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
