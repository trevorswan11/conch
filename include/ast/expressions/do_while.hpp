#pragma once

#include <memory>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement;

class DoWhileLoopExpression : public Expression {
  public:
    explicit DoWhileLoopExpression(const Token&                    start_token,
                                   std::unique_ptr<BlockStatement> block,
                                   std::unique_ptr<Expression>     condition) noexcept;
    ~DoWhileLoopExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<DoWhileLoopExpression>, ParserDiagnostic>;

    [[nodiscard]] auto block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto condition() const noexcept -> const Expression& { return *condition_; }

  private:
    std::unique_ptr<BlockStatement> block_;
    std::unique_ptr<Expression>     condition_;
};

} // namespace ast
