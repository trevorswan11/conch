#include "ast/statements/decl.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/identifier.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto DeclStatement::accept(Visitor& v) const -> void { v.visit(*this); }

} // namespace ast
