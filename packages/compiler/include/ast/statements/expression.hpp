#pragma once

#include <utility>

#include "expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class ExpressionStatement : public StmtBase<ExpressionStatement> {
  public:
    static constexpr auto KIND = NodeKind::EXPRESSION_STATEMENT;

  public:
    explicit ExpressionStatement(const Token& start_token, Box<Expression> expression) noexcept
        : StmtBase{start_token}, expression_{std::move(expression)} {}

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

    [[nodiscard]] auto get_expression() const noexcept -> const Expression& { return *expression_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<ExpressionStatement>(other);
        return *expression_ == *casted.expression_;
    }

  private:
    Box<Expression> expression_;
};

} // namespace conch::ast
