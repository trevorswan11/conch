#include "ast/expressions/range.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto RangeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto RangeExpression::parse(Parser& parser, Box<Expression> lower)
    -> Expected<Box<RangeExpression>, ParserDiagnostic> {
    TODO(parser, lower);
}

} // namespace conch::ast
