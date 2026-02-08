#include "ast/expressions/index.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IndexExpression::parse(Parser& parser, Box<Expression> array)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser, array);
}

} // namespace conch::ast
