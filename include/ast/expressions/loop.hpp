#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class BlockStatement;

struct ForLoopCapture {
    bool                        reference;
    std::unique_ptr<Expression> capture;
};
using CapturedIterable = std::optional<ForLoopCapture>;

class ForLoopExpression : public Expression {
  public:
    explicit ForLoopExpression(
        const Token&                                                  start_token,
        std::vector<std::unique_ptr<Expression>>                      iterables,
        std::vector<std::optional<std::unique_ptr<CapturedIterable>>> captures,
        std::unique_ptr<Statement>                                    block,
        std::unique_ptr<Statement>                                    non_break) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ForLoopExpression>, ParserDiagnostic>;

  private:
    std::vector<std::unique_ptr<Expression>>                      iterables_;
    std::vector<std::optional<std::unique_ptr<CapturedIterable>>> captures_;
    std::unique_ptr<Statement>                                    block_;
    std::unique_ptr<Statement>                                    non_break_;
};

class WhileLoopExpression : public Expression {
  public:
    explicit WhileLoopExpression(const Token&                    start_token,
                                 std::unique_ptr<Expression>     condition,
                                 std::unique_ptr<Expression>     continuation,
                                 std::unique_ptr<BlockStatement> block,
                                 std::unique_ptr<Statement>      non_break) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<WhileLoopExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<Expression>     condition_;
    std::unique_ptr<Expression>     continuation_;
    std::unique_ptr<BlockStatement> block_;
    std::unique_ptr<Statement>      non_break_;
};

class DoWhileLoopExpression : public Expression {
  public:
    explicit DoWhileLoopExpression(const Token&                    start_token,
                                   std::unique_ptr<BlockStatement> block,
                                   std::unique_ptr<Expression>     condition) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<DoWhileLoopExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<BlockStatement> block_;
    std::unique_ptr<Expression>     condition_;
};

class InfiniteLoopExpression : public Expression {
  public:
    explicit InfiniteLoopExpression(const Token&                    start_token,
                                    std::unique_ptr<BlockStatement> block) noexcept;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<InfiniteLoopExpression>, ParserDiagnostic>;

  private:
    std::unique_ptr<BlockStatement> block_;
};

} // namespace ast
