#include <utility>

#include "ast/statements/namespace.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/expressions/scope_resolve.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

NamespaceStatement::NamespaceStatement(const Token&                 start_token,
                                       std::variant<Single, Nested> nspace,
                                       Box<BlockStatement>          block) noexcept
    : Statement{start_token, NodeKind::NAMESPACE_STATEMENT}, namespace_{std::move(nspace)},
      block_{std::move(block)} {}

NamespaceStatement::~NamespaceStatement() = default;

auto NamespaceStatement::accept(Visitor& v) const -> void { v.visit(*this); }

auto NamespaceStatement::parse(Parser& parser)
    -> Expected<Box<NamespaceStatement>, ParserDiagnostic> {
    TODO(parser);
}

auto NamespaceStatement::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted          = as<NamespaceStatement>(other);
    const auto& other_namespace = casted.namespace_;
    const auto  variant_eq      = std::visit(overloaded{
                                           [&other_namespace](const Single& v) {
                                               return *v == *std::get<Single>(other_namespace);
                                           },
                                           [&other_namespace](const Nested& v) {
                                               return *v == *std::get<Nested>(other_namespace);
                                           },
                                       },
                                       namespace_);
    return variant_eq && *block_ == *casted.block_;
}

} // namespace conch::ast
