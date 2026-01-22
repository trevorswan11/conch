#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class InfixExpression : public Expression {
  public:
    explicit InfixExpression(const Token&                start_token,
                             std::unique_ptr<Expression> lhs,
                             TokenType                   op,
                             std::unique_ptr<Expression> rhs) noexcept
        : Expression{start_token}, lhs_{std::move(lhs)}, op_{op}, rhs_{std::move(rhs)} {}

    auto op() const noexcept -> TokenType { return op_; }
    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<InfixExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression> lhs_;
    TokenType                   op_;
    std::unique_ptr<Expression> rhs_;
};

} // namespace ast
