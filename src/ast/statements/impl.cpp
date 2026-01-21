#include <utility>

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"
#include "ast/statements/impl.hpp"

namespace ast {

ImplStatement::ImplStatement(const Token&                          start_token,
                             std::unique_ptr<IdentifierExpression> parent,
                             std::unique_ptr<BlockStatement>       implementation) noexcept
    : Statement{start_token}, parent_{std::move(parent)},
      implementation_{std::move(implementation)} {}

} // namespace ast
