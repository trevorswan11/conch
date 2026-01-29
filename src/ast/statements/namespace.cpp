#include <utility>

#include "ast/statements/namespace.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/scope_resolve.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

NamespaceStatement::NamespaceStatement(const Token&                  start_token,
                                       std::variant<Default, Nested> nspace) noexcept
    : Statement{start_token}, namespace_{std::move(nspace)} {}

NamespaceStatement::~NamespaceStatement() = default;

auto NamespaceStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto NamespaceStatement::parse(Parser& parser)
    -> Expected<Box<NamespaceStatement>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
