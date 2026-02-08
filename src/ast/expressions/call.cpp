#include "ast/expressions/call.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(Parser& parser, Box<Expression> function)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser, function);
}

} // namespace conch::ast
