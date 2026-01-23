#include "ast/expressions/array.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto ArrayExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ArrayExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<ArrayExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
