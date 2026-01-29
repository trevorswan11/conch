#pragma once

#include <utility>

#include "util/common.hpp"

#include "ast/node.hpp"

namespace conch::ast {

class InfixExpression : public Expression {
  public:
    explicit InfixExpression(const Token&    start_token,
                             Box<Expression> lhs,
                             TokenType       op,
                             Box<Expression> rhs) noexcept
        : Expression{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    [[nodiscard]] auto get_lhs() const noexcept -> const Expression& { return *lhs_; }
    auto               get_op() const noexcept -> TokenType { return op_; }
    [[nodiscard]] auto get_rhs() const noexcept -> const Expression& { return *rhs_; }

  private:
    Box<Expression> lhs_;
    TokenType       op_;
    Box<Expression> rhs_;
};

} // namespace conch::ast
