#pragma once

#include "ast/expressions/prefix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class PointerExpression : public PrefixExpression<PointerExpression> {
  public:
    static constexpr auto KIND = NodeKind::POINTER_EXPRESSION;

  public:
    using PrefixExpression::PrefixExpression;
    using PrefixExpression::parse;
};

} // namespace conch::ast
