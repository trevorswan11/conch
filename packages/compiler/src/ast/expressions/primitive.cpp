#include <cmath>
#include <limits>

#include "ast/expressions/primitive.hpp"
#include "ast/visitor.hpp"

namespace conch::ast {

// cppcheck-suppress-begin [constParameterReference, duplInheritedMember]

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto promoted    = start_token.promote();
    if (!promoted) { return make_parser_unexpected(ParserError::MALFORMED_STRING, start_token); }

    return make_box<StringExpression>(start_token, *promoted);
}

auto SignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto SignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto ISizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto UnsignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }
auto USizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto slice       = start_token.slice;

    if (slice.size() != 3 && slice.size() != 4) {
        return make_parser_unexpected(ParserError::MALFORMED_CHARACTER, start_token);
    }

    if (slice[1] != '\\') { return make_box<ByteExpression>(start_token, slice[1]); }

    const auto escaped = slice[2];
    byte       value;
    switch (escaped) {
    case 'n':  value = '\n'; break;
    case 'r':  value = '\r'; break;
    case 't':  value = '\t'; break;
    case '\\': value = '\\'; break;
    case '\'': value = '\''; break;
    case '"':  value = '"'; break;
    case '0':  value = '\0'; break;
    default:   return make_parser_unexpected(ParserError::UNKNOWN_CHARACTER_ESCAPE, start_token);
    }

    return make_box<ByteExpression>(start_token, value);
}

auto FloatExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FloatExpression::is_equal(const Node& other) const noexcept -> bool {
    const auto& casted = as<FloatExpression>(other);
    return approx_eq(value_, casted.value_);
}

auto FloatExpression::approx_eq(value_type a, value_type b) -> bool {
    const auto largest = std::max(std::abs(b), std::abs(a));
    const auto diff    = std::abs(a - b);
    return diff <= largest * std::numeric_limits<value_type>::epsilon();
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto& start_token = parser.current_token();
    return make_box<BoolExpression>(start_token, start_token.type == TokenType::TRUE);
}

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace conch::ast
