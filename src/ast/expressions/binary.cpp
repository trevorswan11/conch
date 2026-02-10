#include "ast/expressions/binary.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto BinaryExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace conch::ast
