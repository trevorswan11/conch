#pragma once

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BlockStatement;

class InfiniteLoopExpression : public ExprBase<InfiniteLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::INFINITE_LOOP_EXPRESSION;

  public:
    explicit InfiniteLoopExpression(const Token& start_token, Box<BlockStatement> block) noexcept;
    ~InfiniteLoopExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_block() const noexcept -> const BlockStatement& { return *block_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<BlockStatement> block_;
};

} // namespace conch::ast
