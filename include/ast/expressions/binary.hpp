#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BinaryExpression : public InfixExpression {
  public:
    explicit BinaryExpression(const Token&    start_token,
                              Box<Expression> lhs,
                              TokenType       op,
                              Box<Expression> rhs) noexcept
        : InfixExpression{
              start_token, NodeKind::BINARY_EXPRESSION, std::move(lhs), op, std::move(rhs)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lhs)
        -> Expected<Box<BinaryExpression>, ParserDiagnostic>;
};

} // namespace conch::ast
