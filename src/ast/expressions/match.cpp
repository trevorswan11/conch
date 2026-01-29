#include "ast/expressions/match.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto MatchExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto MatchExpression::parse(Parser& parser) -> Expected<Box<MatchExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
