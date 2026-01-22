#include "ast/expressions/struct.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto StructExpression::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
