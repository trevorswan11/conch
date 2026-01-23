#pragma once

#include <memory>
#include <optional>
#include <utility>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class JumpStatement : public Statement {
  public:
    explicit JumpStatement(const Token&                               start_token,
                           std::optional<std::unique_ptr<Expression>> expression) noexcept
        : Statement{start_token}, expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<JumpStatement>, ParserDiagnostic>;

    [[nodiscard]] auto expression() const noexcept -> std::optional<const Expression*> {
        if (expression_) { return expression_->get(); }
        return std::nullopt;
    }

  private:
    std::optional<std::unique_ptr<Expression>> expression_;
};

} // namespace ast
