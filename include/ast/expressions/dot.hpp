#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/expressions/infix.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class DotExpression : public InfixExpression<DotExpression> {
  public:
    static constexpr auto KIND = NodeKind::DOT_EXPRESSION;

  public:
    explicit DotExpression(const Token&    start_token,
                           Box<Expression> lhs,
                           Box<Expression> rhs) noexcept
        : InfixExpression{start_token, std::move(lhs), TokenType::DOT, std::move(rhs)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser, Box<Expression> lhs)
        -> Expected<Box<Expression>, ParserDiagnostic>;
};

} // namespace conch::ast
