#pragma once

#include "core/common.hpp"
#include "core/expected.hpp"
#include "core/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class BlockStatement;

class WhileLoopExpression : public ExprBase<WhileLoopExpression> {
  public:
    static constexpr auto KIND = NodeKind::WHILE_LOOP_EXPRESSION;

  public:
    explicit WhileLoopExpression(const Token&              start_token,
                                 Box<Expression>           condition,
                                 Optional<Box<Expression>> continuation,
                                 Box<BlockStatement>       block,
                                 Optional<Box<Statement>>  non_break) noexcept;
    ~WhileLoopExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto get_condition() const noexcept -> const Expression& { return *condition_; }
    [[nodiscard]] auto get_continuation() const noexcept -> Optional<const Expression&> {
        return continuation_ ? Optional<const Expression&>{**continuation_} : nullopt;
    }
    [[nodiscard]] auto get_block() const noexcept -> const BlockStatement& { return *block_; }
    [[nodiscard]] auto has_non_break() const noexcept -> bool { return non_break_.has_value(); }
    [[nodiscard]] auto get_non_break() const noexcept -> Optional<const Statement&> {
        return non_break_ ? Optional<const Statement&>{**non_break_} : nullopt;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Box<Expression>           condition_;
    Optional<Box<Expression>> continuation_;
    Box<BlockStatement>       block_;
    Optional<Box<Statement>>  non_break_;
};

} // namespace conch::ast
