#include "ast/expressions/index.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IndexExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IndexExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<IndexExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
