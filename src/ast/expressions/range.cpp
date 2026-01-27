#include "ast/expressions/range.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto RangeExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto RangeExpression::parse(Parser& parser, std::unique_ptr<Expression> lower)
    -> Expected<std::unique_ptr<RangeExpression>, ParserDiagnostic> {
    TODO(parser, lower);
}

} // namespace ast
