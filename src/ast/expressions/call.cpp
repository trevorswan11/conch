#include "ast/expressions/call.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto CallExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto CallExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<CallExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
