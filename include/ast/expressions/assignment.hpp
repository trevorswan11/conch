#pragma once

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

namespace conch::ast {

class AssignmentExpression : public InfixExpression<AssignmentExpression> {
  public:
    static constexpr auto KIND = NodeKind::ASSIGNMENT_EXPRESSION;

  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    using InfixExpression::parse;
};

} // namespace conch::ast
