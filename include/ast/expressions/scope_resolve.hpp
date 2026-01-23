#pragma once

#include <memory>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;

class ScopeResolutionExpression : public Expression {
  public:
    explicit ScopeResolutionExpression(const Token&                          start_token,
                                       std::unique_ptr<Expression>           outer,
                                       std::unique_ptr<IdentifierExpression> inner) noexcept;
    ~ScopeResolutionExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ScopeResolutionExpression>, ParserDiagnostic>;

    [[nodiscard]] auto outer() const noexcept -> const Expression& { return *outer_; }
    [[nodiscard]] auto inner() const noexcept -> const IdentifierExpression& { return *inner_; }

  private:
    std::unique_ptr<Expression>           outer_;
    std::unique_ptr<IdentifierExpression> inner_;
};

} // namespace ast
