#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class RangeExpression : public InfixExpression<RangeExpression> {
  public:
    static constexpr auto KIND = NodeKind::RANGE_EXPRESSION;

  public:
    explicit RangeExpression(const Token&    start_token,
                             Box<Expression> lower,
                             TokenType       op,
                             Box<Expression> upper) noexcept
        : InfixExpression{start_token, std::move(lower), op, std::move(upper)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lower)
        -> Expected<Box<Expression>, ParserDiagnostic>;
};

} // namespace conch::ast
