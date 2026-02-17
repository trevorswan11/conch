#pragma once

#include "core/common.hpp"
#include "core/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;

class ScopeResolutionExpression : public ExprBase<ScopeResolutionExpression> {
  public:
    static constexpr auto KIND = NodeKind::SCOPE_RESOLUTION_EXPRESSION;

  public:
    explicit ScopeResolutionExpression(const Token&              start_token,
                                       Box<Expression>           outer,
                                       Box<IdentifierExpression> inner) noexcept;
    ~ScopeResolutionExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> outer)
        -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_outer() const noexcept -> const Expression& { return *outer_; }
    [[nodiscard]] auto get_inner() const noexcept -> const IdentifierExpression& { return *inner_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           outer_;
    Box<IdentifierExpression> inner_;
};

} // namespace conch::ast
