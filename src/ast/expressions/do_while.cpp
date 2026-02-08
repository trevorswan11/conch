#include <utility>

#include "ast/expressions/do_while.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

DoWhileLoopExpression::DoWhileLoopExpression(const Token&        start_token,
                                             Box<BlockStatement> block,
                                             Box<Expression>     condition) noexcept
    : KindExpression{start_token}, block_{std::move(block)}, condition_{std::move(condition)} {}

DoWhileLoopExpression::~DoWhileLoopExpression() = default;

auto DoWhileLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DoWhileLoopExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser);
}

auto DoWhileLoopExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<DoWhileLoopExpression>(other);
    return *block_ == *casted.block_ && *condition_ == *casted.condition_;
}

} // namespace conch::ast
