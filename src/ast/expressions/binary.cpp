#include "ast/expressions/binary.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto BinaryExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BinaryExpression::parse(Parser& parser, Box<Expression> lhs)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser, lhs);
}

} // namespace conch::ast
