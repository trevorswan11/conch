#pragma once

#include "ast/expressions/prefix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class ImplicitAccessExpression : public PrefixExpression<ImplicitAccessExpression> {
  public:
    static constexpr auto KIND = NodeKind::IMPLICIT_ACCESS_EXPRESSION;

  public:
    using PrefixExpression::PrefixExpression;
    using PrefixExpression::parse;
};

} // namespace conch::ast
