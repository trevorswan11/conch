#pragma once

#include <memory>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class ExpressionStatement : public Statement {
  public:
    explicit ExpressionStatement(const Token&                start_token,
                                 std::unique_ptr<Expression> expression) noexcept
        : Statement{start_token}, expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ExpressionStatement>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression> expression_;
};

} // namespace ast
