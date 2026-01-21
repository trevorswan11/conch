#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto BlockStatement::accept(Visitor& v) -> void { v.visit(*this); }

} // namespace ast
