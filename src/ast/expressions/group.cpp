#include "ast/expressions/group.hpp"

namespace ast {

auto GroupedExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<Expression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
