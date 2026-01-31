#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class PrefixExpression : public Expression {
  public:
    explicit PrefixExpression(const Token& op, Box<Expression> rhs) noexcept
        : Expression{op, NodeKind::PREFIX_EXPRESSION}, rhs_{std::move(rhs)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<PrefixExpression>, ParserDiagnostic>;

    auto               get_op() const noexcept -> TokenType { return start_token_.type; }
    [[nodiscard]] auto get_rhs() const noexcept -> const Expression& { return *rhs_; }

  private:
    Box<Expression> rhs_;
};

} // namespace conch::ast
