#include "ast/statements/discard.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto DiscardStatement::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
