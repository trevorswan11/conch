#include "ast/expressions/dot.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto DotExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto DotExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<DotExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
