#include "ast/expressions/group.hpp"

namespace conch::ast {

auto GroupedExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
