#include <utility>

#include "ast/expressions/while.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

WhileLoopExpression::WhileLoopExpression(const Token&             start_token,
                                         Box<Expression>          condition,
                                         Box<Expression>          continuation,
                                         Box<BlockStatement>      block,
                                         Optional<Box<Statement>> non_break) noexcept
    : Expression{start_token}, condition_{std::move(condition)},
      continuation_{std::move(continuation)}, block_{std::move(block)},
      non_break_{std::move(non_break)} {}

WhileLoopExpression::~WhileLoopExpression() = default;

auto WhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto WhileLoopExpression::parse(Parser& parser)
    -> Expected<Box<WhileLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
