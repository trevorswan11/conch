#include "ast/expressions/binary.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto BinaryExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BinaryExpression::parse(Parser& parser, std::unique_ptr<Expression> lhs)
    -> Expected<std::unique_ptr<BinaryExpression>, ParserDiagnostic> {
    TODO(parser, lhs);
}

} // namespace ast
