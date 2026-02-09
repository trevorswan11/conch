#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BinaryExpression : public InfixExpression<BinaryExpression> {
  public:
    static constexpr auto KIND = NodeKind::BINARY_EXPRESSION;

  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lhs)
        -> Expected<Box<Expression>, ParserDiagnostic>;
};

} // namespace conch::ast
