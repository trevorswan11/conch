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

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser) -> Expected<std::unique_ptr<JumpStatement>, ParserDiagnostic>;

  private:
    std::optional<std::unique_ptr<Expression>> expression_;
};

} // namespace ast
