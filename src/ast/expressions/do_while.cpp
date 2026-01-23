#include <utility>

#include "ast/expressions/do_while.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

DoWhileLoopExpression::DoWhileLoopExpression(const Token&                    start_token,
                                             std::unique_ptr<BlockStatement> block,
                                             std::unique_ptr<Expression>     condition) noexcept
    : Expression{start_token}, block_{std::move(block)}, condition_{std::move(condition)} {}

DoWhileLoopExpression::~DoWhileLoopExpression() = default;

auto DoWhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DoWhileLoopExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<DoWhileLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
