#include <utility>

#include "ast/statements/impl.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ImplStatement::ImplStatement(const Token&                          start_token,
                             std::unique_ptr<IdentifierExpression> parent,
                             std::unique_ptr<BlockStatement>       implementation) noexcept
    : Statement{start_token}, parent_{std::move(parent)},
      implementation_{std::move(implementation)} {}

ImplStatement::~ImplStatement() = default;

auto ImplStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto ImplStatement::parse(Parser& parser)
    -> Expected<std::unique_ptr<ImplStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
