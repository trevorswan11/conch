#include <charconv>
#include <utility>

#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser) -> Expected<Box<StringExpression>, ParserDiagnostic> {
    TODO(parser);
}

auto SignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SignedIntegerExpression::parse(Parser& parser)
    -> Expected<Box<SignedIntegerExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto base        = token_type::to_base(start_token.type);

    value_type v;
    const auto& [_, ec] = std::from_chars(
        start_token.slice.cbegin(), start_token.slice.cend(), v, std::to_underlying(base));
    switch (ec) {
    case std::errc::result_out_of_range:
        return make_parser_unexpected(ParserError::PRIMITIVE_OVERFLOW, start_token);
    case std::errc::invalid_argument:
        return make_parser_unexpected(ParserError::PRIMITIVE_PARSE_ERROR, start_token);
    default:
        if (ec == std::errc{}) { break; }
        std::unreachable();
    }
}

auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnsignedIntegerExpression::parse(Parser& parser)
    -> Expected<Box<UnsignedIntegerExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto base        = token_type::to_base(start_token.type);
}

auto SizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SizeIntegerExpression::parse(Parser& parser)
    -> Expected<Box<SizeIntegerExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto base        = token_type::to_base(start_token.type);
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
