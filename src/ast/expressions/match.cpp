#include "ast/expressions/match.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto MatchExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<MatchExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
