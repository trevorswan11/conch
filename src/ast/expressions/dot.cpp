#include "ast/expressions/dot.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto DotExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DotExpression::parse(Parser& parser, Box<Expression> lhs)
    -> Expected<Box<DotExpression>, ParserDiagnostic> {
    TODO(parser, lhs);
}

} // namespace conch::ast
