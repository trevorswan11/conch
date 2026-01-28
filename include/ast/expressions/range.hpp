#pragma once

#include <memory>

#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class RangeExpression : public InfixExpression {
  public:
    using InfixExpression::InfixExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, std::unique_ptr<Expression> lower)
        -> Expected<std::unique_ptr<RangeExpression>, ParserDiagnostic>;
};

} // namespace ast
