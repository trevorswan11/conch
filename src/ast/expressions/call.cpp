#include "ast/expressions/call.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(Parser& parser, std::unique_ptr<Expression> function)
    -> Expected<std::unique_ptr<CallExpression>, ParserDiagnostic> {
    TODO(parser, function);
}

} // namespace ast
