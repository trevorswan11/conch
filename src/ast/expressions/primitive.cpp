#include <charconv>
#include <utility>

#include "ast/expressions/primitive.hpp"

#include "visitor/visitor.hpp"

namespace conch::ast {

// cppcheck-suppress-begin constParameterReference

auto StringExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto StringExpression::parse(Parser& parser) -> Expected<Box<StringExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto promoted    = start_token.promote();
    if (!promoted) { return make_parser_unexpected(ParserError::MALFORMED_STRING, start_token); }

    return make_box<StringExpression>(start_token, *promoted);
}

template <typename T>
static auto parse_number(Parser& parser) -> Expected<Box<T>, ParserDiagnostic> {
    using value_type                 = typename T::value_type;
    constexpr auto is_floating_point = std::is_same_v<value_type, double>;
    const auto     start_token       = parser.current_token();
    const auto     base              = token_type::to_base(start_token.type);

    const auto* first = start_token.slice.cbegin() + (base == Base::DECIMAL ? 0 : 2);
    const auto* last  = start_token.slice.cend();

    value_type             v;
    std::from_chars_result result;
    if constexpr (is_floating_point) {
        result = std::from_chars(first, last, v);
    } else {
        result = std::from_chars(first, last, v, std::to_underlying(base));
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

auto SignedIntegerExpression::parse(Parser& parser)
    -> Expected<Box<SignedIntegerExpression>, ParserDiagnostic> {
    return parse_number<SignedIntegerExpression>(parser);
}

auto UnsignedIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto UnsignedIntegerExpression::parse(Parser& parser)
    -> Expected<Box<UnsignedIntegerExpression>, ParserDiagnostic> {
    return parse_number<UnsignedIntegerExpression>(parser);
}

auto SizeIntegerExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto SizeIntegerExpression::parse(Parser& parser)
    -> Expected<Box<SizeIntegerExpression>, ParserDiagnostic> {
    return parse_number<SizeIntegerExpression>(parser);
}

auto ByteExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto ByteExpression::parse(Parser& parser) -> Expected<Box<ByteExpression>, ParserDiagnostic> {
    const auto start_token = parser.current_token();
    const auto slice       = start_token.slice;

    if (slice.size() != 3 && slice.size() != 4) {
        return make_parser_unexpected(ParserError::MALFORMED_CHARACTER, start_token);
    }

    if (slice[1] != '\\') { return make_box<ByteExpression>(start_token, slice[1]); }

    const auto escaped = slice[2];
    byte       value;
    switch (escaped) {
    case 'n': value = '\n'; break;
    case 'r': value = '\r'; break;
    case 't': value = '\t'; break;
    case '\\': value = '\\'; break;
    case '\'': value = '\''; break;
    case '"': value = '"'; break;
    case '0': value = '\0'; break;
    default: return make_parser_unexpected(ParserError::UNKNOWN_CHARACTER_ESCAPE, start_token);
    }

    return make_box<ByteExpression>(start_token, value);
}

auto FloatExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto FloatExpression::parse(Parser& parser) -> Expected<Box<FloatExpression>, ParserDiagnostic> {
    return parse_number<FloatExpression>(parser);
}

auto BoolExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto BoolExpression::parse(Parser& parser) -> Expected<Box<BoolExpression>, ParserDiagnostic> {
    const auto& start_token = parser.current_token();
    return make_box<BoolExpression>(start_token, start_token.type == TokenType::TRUE);
}

auto VoidExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto VoidExpression::parse(Parser& parser) -> Expected<Box<VoidExpression>, ParserDiagnostic> {
    return make_box<VoidExpression>(parser.current_token(), std::monostate{});
}

auto NilExpression::accept(Visitor& v) const -> void { v.visit(*this); }

auto NilExpression::parse(Parser& parser) -> Expected<Box<NilExpression>, ParserDiagnostic> {
    return make_box<NilExpression>(parser.current_token(), std::monostate{});
}

// cppcheck-suppress-end constParameterReference

} // namespace conch::ast
