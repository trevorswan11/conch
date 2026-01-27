#pragma once

#include <memory>
#include <utility>

#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class JumpStatement : public Statement {
  public:
    explicit JumpStatement(const Token&                          start_token,
                           Optional<std::unique_ptr<Expression>> expression) noexcept
        : Statement{start_token}, expression_{std::move(expression)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<JumpStatement>, ParserDiagnostic>;

    [[nodiscard]] auto expression() const noexcept -> Optional<const Expression&> {
        return expression_ ? Optional<const Expression&>{**expression_} : nullopt;
    }

  private:
    Optional<std::unique_ptr<Expression>> expression_;
};

} // namespace ast
