#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class PrefixExpression : public Expression {
  public:
    explicit PrefixExpression(const Token& op, std::unique_ptr<Expression> rhs) noexcept
        : Expression{op}, rhs_{std::move(rhs)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<PrefixExpression>, ParserDiagnostic>;

    auto op() const noexcept -> TokenType { return start_token_.type; }

  private:
    std::unique_ptr<Expression> rhs_;
};

} // namespace ast
