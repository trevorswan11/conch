#include "ast/expressions/if.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto IfExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IfExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<IfExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
