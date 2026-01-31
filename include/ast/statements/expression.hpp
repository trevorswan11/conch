#pragma once

#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class ExpressionStatement : public Statement {
  public:
    explicit ExpressionStatement(const Token& start_token, Box<Expression> expression) noexcept
        : Statement{start_token, NodeKind::EXPRESSION_STATEMENT},
          expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<ExpressionStatement>, ParserDiagnostic>;

    [[nodiscard]] auto get_expression() const noexcept -> const Expression& { return *expression_; }

  private:
    Box<Expression> expression_;
};

} // namespace conch::ast
