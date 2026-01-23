#include <utility>

#include "ast/expressions/for.hpp"

#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace ast {

ForLoopExpression::ForLoopExpression(const Token&                               start_token,
                                     std::vector<std::unique_ptr<Expression>>   iterables,
                                     std::vector<std::optional<ForLoopCapture>> captures,
                                     std::unique_ptr<BlockStatement>            block,
                                     std::unique_ptr<Statement>                 non_break) noexcept
    : Expression{start_token}, iterables_{std::move(iterables)}, captures_{std::move(captures)},
      block_{std::move(block)}, non_break_{std::move(non_break)} {}

ForLoopExpression::~ForLoopExpression() = default;

auto ForLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<ForLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
