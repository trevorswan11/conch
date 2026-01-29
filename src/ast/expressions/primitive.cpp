#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser) -> Expected<Box<StringExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto IntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto IntegerExpression::parse(Parser& parser)
    -> Expected<Box<IntegerExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto ByteExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::parse(Parser& parser) -> Expected<Box<ByteExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto FloatExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FloatExpression::parse(Parser& parser) -> Expected<Box<FloatExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(Parser& parser) -> Expected<Box<BoolExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto VoidExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto VoidExpression::parse(Parser& parser) -> Expected<Box<VoidExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto NilExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto NilExpression::parse(Parser& parser) -> Expected<Box<NilExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace conch::ast
