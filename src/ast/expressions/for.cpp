#include <utility>

#include "ast/expressions/for.hpp"

#include "ast/expressions/identifier.hpp"
#include "ast/statements/block.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

ForLoopCapture::ForLoopCapture(bool reference, Box<IdentifierExpression> capture) noexcept
    : reference_{reference}, capture_{std::move(capture)} {}

ForLoopCapture::~ForLoopCapture() = default;

ForLoopExpression::ForLoopExpression(const Token&                          start_token,
                                     std::vector<Box<Expression>>          iterables,
                                     std::vector<Optional<ForLoopCapture>> captures,
                                     Box<BlockStatement>                   block,
                                     Box<Statement>                        non_break) noexcept
    : Expression{start_token, NodeKind::FOR_LOOP_EXPRESSION}, iterables_{std::move(iterables)},
      captures_{std::move(captures)}, block_{std::move(block)}, non_break_{std::move(non_break)} {}

ForLoopExpression::~ForLoopExpression() = default;

auto ForLoopExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ForLoopExpression::parse(Parser& parser)
    -> Expected<Box<ForLoopExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
