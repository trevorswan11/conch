#include "ast/statements/jump.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto JumpStatement::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
