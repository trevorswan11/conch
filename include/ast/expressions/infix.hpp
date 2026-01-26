#pragma once

#include <memory>
#include <utility>

#include "ast/node.hpp"

namespace ast {

class InfixExpression : public Expression {
  public:
    explicit InfixExpression(const Token&                start_token,
                             std::unique_ptr<Expression> lhs,
                             TokenType                   op,
                             std::unique_ptr<Expression> rhs) noexcept
        : Expression{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    [[nodiscard]] auto lhs() const noexcept -> const Expression& { return *lhs_; }
    auto               op() const noexcept -> TokenType { return op_; }
    [[nodiscard]] auto rhs() const noexcept -> const Expression& { return *rhs_; }

  private:
    std::unique_ptr<Expression> lhs_;
    TokenType                   op_;
    std::unique_ptr<Expression> rhs_;
};

} // namespace ast
