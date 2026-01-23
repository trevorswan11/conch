#include <utility>

#include "ast/expressions/infinite_loop.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

InfiniteLoopExpression::InfiniteLoopExpression(const Token&                    start_token,
                                               std::unique_ptr<BlockStatement> block) noexcept
    : Expression{start_token}, block_{std::move(block)} {}

InfiniteLoopExpression::~InfiniteLoopExpression() = default;

auto InfiniteLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto InfiniteLoopExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<InfiniteLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
