#pragma once

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BlockStatement;

class DoWhileLoopExpression : public ExprBase<DoWhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::DO_WHILE_LOOP_EXPRESSION;

  public:
    explicit DoWhileLoopExpression(const Token&        start_token,
                                   Box<BlockStatement> block,
                                   Box<Expression>     condition) noexcept;
    ~DoWhileLoopExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto get_condition() const noexcept -> const Expression& { return *condition_; }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<BlockStatement> block_;
    Box<Expression>     condition_;
};

} // namespace conch::ast
