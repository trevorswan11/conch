#pragma once

#include <memory>
#include <optional>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement;

class WhileLoopExpression : public Expression {
  public:
    explicit WhileLoopExpression(const Token&                    start_token,
                                 std::unique_ptr<Expression>     condition,
                                 std::unique_ptr<Expression>     continuation,
                                 std::unique_ptr<BlockStatement> block) noexcept;
    explicit WhileLoopExpression(const Token&                              start_token,
                                 std::unique_ptr<Expression>               condition,
                                 std::unique_ptr<Expression>               continuation,
                                 std::unique_ptr<BlockStatement>           block,
                                 std::optional<std::unique_ptr<Statement>> non_break) noexcept;
    ~WhileLoopExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<WhileLoopExpression>, ParserDiagnostic>;

    [[nodiscard]] auto condition() const noexcept -> const Expression& { return *condition_; }
    [[nodiscard]] auto continuation() const noexcept -> const Expression& { return *continuation_; }
    [[nodiscard]] auto block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto non_break() const noexcept -> std::optional<const Statement*> {
        if (non_break_) { return non_break_->get(); }
        return std::nullopt;
    }

  private:
    std::unique_ptr<Expression>               condition_;
    std::unique_ptr<Expression>               continuation_;
    std::unique_ptr<BlockStatement>           block_;
    std::optional<std::unique_ptr<Statement>> non_break_;
};

} // namespace ast
