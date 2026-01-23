#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace ast {

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<StringExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto SignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SignedIntegerExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<SignedIntegerExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnsignedIntegerExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<UnsignedIntegerExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto SizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SizeIntegerExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<SizeIntegerExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto ByteExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<ByteExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto FloatExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FloatExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<FloatExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<BoolExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto VoidExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto VoidExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<VoidExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto NilExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto NilExpression::parse(Parser& parser)
    -> Expected<std::unique_ptr<NilExpression>, ParserDiagnostic> {
    TODO(parser);
}

} // namespace ast
