#include "ast/expressions/prefix.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto PrefixExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto PrefixExpression::parse(Parser& parser) -> Expected<Box<PrefixExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
