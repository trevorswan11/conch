#include <utility>

#include "ast/expressions/infinite_loop.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

InfiniteLoopExpression::InfiniteLoopExpression(const Token&        start_token,
                                               Box<BlockStatement> block) noexcept
    : KindExpression{start_token}, block_{std::move(block)} {}

InfiniteLoopExpression::~InfiniteLoopExpression() = default;

auto InfiniteLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto InfiniteLoopExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser);
}

auto InfiniteLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<InfiniteLoopExpression>(other);
    return *block_ == *casted.block_;
}

} // namespace conch::ast
