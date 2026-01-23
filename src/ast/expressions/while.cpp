#include <utility>

#include "ast/expressions/while.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

WhileLoopExpression::WhileLoopExpression(const Token&                    start_token,
                                         std::unique_ptr<Expression>     condition,
                                         std::unique_ptr<Expression>     continuation,
                                         std::unique_ptr<BlockStatement> block) noexcept
    : WhileLoopExpression{start_token,
                          std::move(condition),
                          std::move(continuation),
                          std::move(block),
                          std::nullopt} {}

WhileLoopExpression::WhileLoopExpression(
    const Token&                              start_token,
    std::unique_ptr<Expression>               condition,
    std::unique_ptr<Expression>               continuation,
    std::unique_ptr<BlockStatement>           block,
    std::optional<std::unique_ptr<Statement>> non_break) noexcept
    : Expression{start_token}, condition_{std::move(condition)},
      continuation_{std::move(continuation)}, block_{std::move(block)},
      non_break_{std::move(non_break)} {}

WhileLoopExpression::~WhileLoopExpression() = default;

auto WhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto WhileLoopExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<WhileLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
