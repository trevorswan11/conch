#include "ast/expressions/array.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ArrayExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
