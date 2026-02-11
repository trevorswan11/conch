#include "ast/expressions/if.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IfExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
