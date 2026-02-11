#include <charconv>
#include <cmath>
#include <limits>
#include <utility>

#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

// cppcheck-suppress-begin constParameterReference

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto promoted    = start_token.promote();
    if (!promoted) { return make_parser_unexpected(ParserError::MALFORMED_STRING, start_token); }

    return make_box<StringExpression>(start_token, *promoted);
}

template <typename T>
static auto parse_number(Parser& parser) -> Expected<Box<T>, ParserDiagnostic> {
    using value_type                 = typename T::value_type;
    constexpr auto is_floating_point = std::is_same_v<value_type, f64>;
    const auto     start_token       = parser.current_token();
    const auto     base              = token_type::to_base(start_token.type);

    const auto trim_amount = [](TokenType tt) -> usize {
        if (token_type::is_usize_int(tt) || token_type::is_unsigned_long_int(tt)) { return 2; }
        if (token_type::is_isize_int(tt) || token_type::is_signed_long_int(tt) ||
            token_type::is_unsigned_int(tt)) {
            return 1;
        }
        return 0;
    };

    const auto* first = start_token.slice.cbegin() + (!base || *base == Base::DECIMAL ? 0 : 2);
    const auto* last  = start_token.slice.cend() - trim_amount(start_token.type);

    value_type             v;
    std::from_chars_result result;
    if constexpr (is_floating_point) {
        result = std::from_chars(first, last, v);
    } else {
        result = std::from_chars(first, last, v, std::to_underlying(*base));
    }
    if (result.ec == std::errc{} && result.ptr == last) { return make_box<T>(start_token, v); }

    if (result.ec == std::errc::result_out_of_range) {
        const auto err =
            is_floating_point ? ParserError::FLOAT_OVERFLOW : ParserError::INTEGER_OVERFLOW;
        return make_parser_unexpected(err, start_token);
    }

    const auto err =
        is_floating_point ? ParserError::MALFORMED_FLOAT : ParserError::MALFORMED_INTEGER;
    return make_parser_unexpected(err, start_token);
}

auto SignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SignedIntegerExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<SignedIntegerExpression>(parser);
}

auto SignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SignedLongIntegerExpression::parse(Parser& parser)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<SignedLongIntegerExpression>(parser);
}

auto ISizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ISizeIntegerExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<ISizeIntegerExpression>(parser);
}

auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnsignedIntegerExpression::parse(Parser& parser)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<UnsignedIntegerExpression>(parser);
}

auto UnsignedLongIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnsignedLongIntegerExpression::parse(Parser& parser)
    -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<UnsignedLongIntegerExpression>(parser);
}

auto USizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto USizeIntegerExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<USizeIntegerExpression>(parser);
}

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

auto FloatExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    return parse_number<FloatExpression>(parser);
}

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

auto NilExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto NilExpression::parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic> {
    return make_box<NilExpression>(parser.current_token(), std::monostate{});
}

// cppcheck-suppress-end constParameterReference

} // namespace conch::ast
