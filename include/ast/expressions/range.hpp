#pragma once

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class RangeExpression : public InfixExpression<RangeExpression> {
  public:
    static constexpr auto KIND = NodeKind::RANGE_EXPRESSION;

  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    using InfixExpression::parse;
};

} // namespace conch::ast
