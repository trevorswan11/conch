#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

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

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    auto               accept(Visitor& v) const -> void override;
    [[nodiscard]] auto get_block() const noexcept -> const BlockStatement& { return *block_; }
    auto               is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<BlockStatement> block_;
};

} // namespace conch::ast
