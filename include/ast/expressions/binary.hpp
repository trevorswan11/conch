#pragma once

#include <memory>

#include "core.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BinaryExpression : public InfixExpression {
  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, std::unique_ptr<Expression> lhs)
        -> Expected<std::unique_ptr<BinaryExpression>, ParserDiagnostic>;
};

} // namespace ast
