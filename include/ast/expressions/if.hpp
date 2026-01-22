#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IfExpression : public Expression {
  public:
    explicit IfExpression(const Token&                start_token,
                          std::unique_ptr<Expression> condition,
                          std::unique_ptr<Statement>  consequence,
                          std::unique_ptr<Statement>  alternate) noexcept
        : Expression{start_token}, condition_{std::move(condition)},
          consequence_{std::move(consequence)}, alternate_{std::move(alternate)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser) -> Expected<std::unique_ptr<IfExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Statement>  consequence_;
    std::unique_ptr<Statement>  alternate_;
};

} // namespace ast
