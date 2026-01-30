#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class AssignmentExpression : public InfixExpression {
  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> assignee)
        -> Expected<Box<AssignmentExpression>, ParserDiagnostic>;
};

} // namespace conch::ast
