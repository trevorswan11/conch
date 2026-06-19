#include "ast/primitive.hh"

#include <charconv>
#include <string_view>
#include <system_error>
#include <utility>

#include <stdx/assert.hh>
#include <stdx/fixed/vector.hh>
#include <stdx/option.hh>
#include <stdx/profiler.hh>
#include <stdx/result.hh>
#include <stdx/type_traits.hh>
#include <stdx/types.hh>

#include "ast/handle.hh"
#include "syntax/error.hh"
#include "syntax/parser.hh"
#include "syntax/token_type.hh"

namespace ghoti::ast {

namespace {

// A global buffer for storing underscore-cleaned numeric tokens for `std::from_chars`
constinit stdx::fixed::vector<char, 1'024> numeric_buffer;

// Parses the requested value from the string, asserting the from_chars result if requested
template <typename ValueType>
[[nodiscard]] auto parse_primitive_value(std::string_view slice, syntax::TokenType type) noexcept
    -> stdx::option<ValueType> {
    const auto base{syntax::token_type::to_base(type)};
    {
        // This is narrowly scoped to allow the first and last pointer names to be reused
        const auto* first = slice.cbegin() + (!base || *base == syntax::Base::DECIMAL ? 0 : 2);
        const auto* last  = slice.cend() - syntax::token_type::suffix_length(type);

        // Strip out the underscores from the slice
        VERIFY(static_cast<usize>(last - first) < numeric_buffer.capacity(), "Literal too long");
        numeric_buffer.clear();
        for (const auto* ptr = first; ptr != last; ++ptr) {
            if (*ptr != '_') { numeric_buffer.emplace_back(*ptr); }
        }
    }

    // The fixed buffer's end pointer is based on the current size, not underlying capacity
    const auto* first = numeric_buffer.begin();
    const auto* last  = numeric_buffer.end();

    ValueType              v;
    std::from_chars_result result;
    if constexpr (std::floating_point<ValueType>) {
        result = std::from_chars(first, last, v);
    } else {
        result = std::from_chars(first, last, v, std::to_underlying(*base));
    }

    // There should only be one error case, hence the use of option vs. result
    if (result.ec == std::errc{} && result.ptr == last) { return v; }
    ASSERT(result.ec == std::errc::result_out_of_range);
    return stdx::none;
}

template <traits::ValuedPrimitive Primitive>
auto parse_primitive(syntax::Parser& parser) -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    using value_type = typename Primitive::value_type;
    const auto start_token{parser.get_current_token()};
    const auto value{parse_primitive_value<value_type>(start_token.slice, start_token.type)};
    if (value) { return parser.add_expr<Primitive>(start_token, *value); }

    syntax::Error error_code;
    if constexpr (std::is_same_v<value_type, f64>) {
        error_code = syntax::Error::DOUBLE_OVERFLOW;
    } else if constexpr (std::is_same_v<value_type, f32>) {
        error_code = syntax::Error::FLOAT_OVERFLOW;
    } else {
        error_code = syntax::Error::INTEGER_OVERFLOW;
    }
    return make_syntax_err("Overflow of literal", error_code, start_token);
}

} // namespace

#define MAKE_PRIMITIVE_PARSER(Type)                             \
    auto Type::parse(syntax::Parser& parser)                    \
        -> stdx::result<ExpressionHandle, syntax::Diagnostic> { \
        PROFILE_FUNCTION();                                     \
        return parse_primitive<Type>(parser);                   \
    }

MAKE_PRIMITIVE_PARSER(I32Expression)
MAKE_PRIMITIVE_PARSER(I64Expression)
MAKE_PRIMITIVE_PARSER(ISizeExpression)
MAKE_PRIMITIVE_PARSER(U32Expression)
MAKE_PRIMITIVE_PARSER(U64Expression)
MAKE_PRIMITIVE_PARSER(USizeExpression)

auto U8Expression::parse(syntax::Parser& parser)
    -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    const auto start_token{parser.get_current_token()};
    const auto slice{start_token.slice};
    if (slice[1] != '\\') {
        return parser.add_expr<U8Expression>(start_token, static_cast<u8>(slice[1]));
    }

    const auto escaped{slice[2]};
    u8         value;
    switch (escaped) {
    case 'n':  value = '\n'; break;
    case 'r':  value = '\r'; break;
    case 't':  value = '\t'; break;
    case '\\': value = '\\'; break;
    case '\'': value = '\''; break;
    case '0':  value = '\0'; break;
    default:   return make_syntax_err(syntax::Error::UNKNOWN_CHARACTER_ESCAPE, start_token);
    }

    return parser.add_expr<U8Expression>(start_token, value);
}

MAKE_PRIMITIVE_PARSER(F32Expression)
MAKE_PRIMITIVE_PARSER(F64Expression)

auto BoolExpression::parse(syntax::Parser& parser)
    -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    return parser.add_expr<BoolExpression>(parser.get_current_token());
}

auto VoidExpression::parse(syntax::Parser& parser)
    -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    const auto start_token{parser.get_current_token()};
    TRY(parser.expect_peek(syntax::TokenType::RBRACE));
    return parser.add_expr<VoidExpression>(start_token);
}

auto UndefinedExpression::parse(syntax::Parser& parser)
    -> stdx::result<ExpressionHandle, syntax::Diagnostic> {
    PROFILE_FUNCTION();
    return parser.add_expr<UndefinedExpression>(parser.get_current_token());
}

} // namespace ghoti::ast
