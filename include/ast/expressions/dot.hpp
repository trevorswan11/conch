#pragma once

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class DotExpression : public InfixExpression<DotExpression> {
  public:
    static constexpr auto KIND = NodeKind::DOT_EXPRESSION;

  public:
    using InfixExpression::InfixExpression;
    using InfixExpression::parse;
};

} // namespace conch::ast
