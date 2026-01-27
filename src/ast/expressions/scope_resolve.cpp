#include <utility>

#include "ast/expressions/scope_resolve.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ScopeResolutionExpression::ScopeResolutionExpression(
    const Token&                          start_token,
    std::unique_ptr<Expression>           outer,
    std::unique_ptr<IdentifierExpression> inner) noexcept
    : Expression{start_token}, outer_{std::move(outer)}, inner_{std::move(inner)} {}

ScopeResolutionExpression::~ScopeResolutionExpression() = default;

auto ScopeResolutionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ScopeResolutionExpression::parse(Parser& parser, std::unique_ptr<Expression> outer)
    -> Expected<std::unique_ptr<ScopeResolutionExpression>, ParserDiagnostic> {
    TODO(parser, outer);
}

} // namespace ast
