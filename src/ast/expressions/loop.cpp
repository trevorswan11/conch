#include "ast/expressions/loop.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto InfiniteLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
