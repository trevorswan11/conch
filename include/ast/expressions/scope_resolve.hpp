#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;

class ScopeResolutionExpression : public Expression {
  public:
    explicit ScopeResolutionExpression(const Token&              start_token,
                                       Box<Expression>           outer,
                                       Box<IdentifierExpression> inner) noexcept;
    ~ScopeResolutionExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> outer)
        -> Expected<Box<ScopeResolutionExpression>, ParserDiagnostic>;

    [[nodiscard]] auto get_outer() const noexcept -> const Expression& { return *outer_; }
    [[nodiscard]] auto get_inner() const noexcept -> const IdentifierExpression& { return *inner_; }

    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           outer_;
    Box<IdentifierExpression> inner_;
};

} // namespace conch::ast
