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

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ScopeResolutionExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression>           outer_;
    std::unique_ptr<IdentifierExpression> inner_;
};

} // namespace ast
