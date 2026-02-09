#include <utility>

#include "ast/expressions/scope_resolve.hpp"

#include "ast/expressions/identifier.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

ScopeResolutionExpression::ScopeResolutionExpression(const Token&              start_token,
                                                     Box<Expression>           outer,
                                                     Box<IdentifierExpression> inner) noexcept
    : ExprBase{start_token}, outer_{std::move(outer)}, inner_{std::move(inner)} {}

ScopeResolutionExpression::~ScopeResolutionExpression() = default;

auto ScopeResolutionExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ScopeResolutionExpression::parse(Parser& parser, Box<Expression> outer)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser, outer);
}

auto ScopeResolutionExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<ScopeResolutionExpression>(other);
    return *outer_ == *casted.outer_ && *inner_ == *casted.inner_;
}

} // namespace conch::ast
