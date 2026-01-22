#include "ast/expressions/scope_resolve.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ScopeResolutionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
