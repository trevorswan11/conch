#pragma once

#include <charconv>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

// cppcheck-suppress-begin [constParameterReference, duplInheritedMember]

template <typename T> struct disable_default_parse : std::false_type {};

// A necessarily instantiable Node with an underlying primitive value type.
template <typename N>
concept PrimitiveNode = LeafNode<N> && requires { typename N::value_type; };

template <typename Derived, typename T> class PrimitiveExpression : public ExprBase<Derived> {
  public:
    using value_type = T;

  public:
    explicit PrimitiveExpression(const Token& start_token, value_type value) noexcept
        : ExprBase<Derived>{start_token}, value_{std::move(value)} {}

    static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>
        requires(!disable_default_parse<Derived>::value)
    {
        const auto start_token = parser.current_token();
        const auto base        = token_type::to_base(start_token.type);

        const auto* first = start_token.slice.cbegin() + (!base || *base == Base::DECIMAL ? 0 : 2);
        const auto* last  = start_token.slice.cend() - token_type::suffix_length(start_token.type);

        value_type             v;
        std::from_chars_result result;
        if constexpr (std::is_same_v<value_type, f64> || std::is_same_v<value_type, f32>) {
            result = std::from_chars(first, last, v);
        } else {
            result = std::from_chars(first, last, v, std::to_underlying(*base));
        }
        if (result.ec == std::errc{} && result.ptr == last) {
            return make_box<Derived>(start_token, v);
        }

        if (result.ec == std::errc::result_out_of_range) {
            return make_parser_unexpected(std::is_same_v<value_type, f64>
                                              ? ParserError::DOUBLE_OVERFLOW
                                              : (std::is_same_v<value_type, f32>
                                                     ? ParserError::FLOAT_OVERFLOW
                                                     : ParserError::INTEGER_OVERFLOW),
                                          start_token);
        }

        return make_parser_unexpected(std::is_same_v<value_type, f64>
                                          ? ParserError::MALFORMED_DOUBLE
                                          : (std::is_same_v<value_type, f32>
                                                 ? ParserError::MALFORMED_FLOAT
                                                 : ParserError::MALFORMED_INTEGER),
                                      start_token);
    }

    MAKE_AST_GETTER(value, const value_type&, )

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = Node::as<Derived>(other);
        return value_ == casted.value_;
    }

  protected:
    value_type value_;
};

class StringExpression : public PrimitiveExpression<StringExpression, std::string> {
  public:
    static constexpr auto KIND = NodeKind::STRING_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(StringExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};
template <> struct disable_default_parse<StringExpression> : std::true_type {};

class SignedIntegerExpression : public PrimitiveExpression<SignedIntegerExpression, i32> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(SignedIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class SignedLongIntegerExpression : public PrimitiveExpression<SignedLongIntegerExpression, i64> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(SignedLongIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class ISizeIntegerExpression : public PrimitiveExpression<ISizeIntegerExpression, isize> {
  public:
    static constexpr auto KIND = NodeKind::ISIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(ISizeIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class UnsignedIntegerExpression : public PrimitiveExpression<UnsignedIntegerExpression, u32> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(UnsignedIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class UnsignedLongIntegerExpression
    : public PrimitiveExpression<UnsignedLongIntegerExpression, u64> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(UnsignedLongIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class USizeIntegerExpression : public PrimitiveExpression<USizeIntegerExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::USIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(USizeIntegerExpression)

    auto accept(Visitor& v) const -> void override;
};

class ByteExpression : public PrimitiveExpression<ByteExpression, byte> {
  public:
    static constexpr auto KIND = NodeKind::BYTE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(ByteExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};
template <> struct disable_default_parse<ByteExpression> : std::true_type {};

class FloatExpression : public PrimitiveExpression<FloatExpression, f32> {
  public:
    static constexpr auto KIND = NodeKind::FLOAT_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(FloatExpression)

    auto accept(Visitor& v) const -> void override;

    auto is_equal(const Node& other) const noexcept -> bool override;
};

class DoubleExpression : public PrimitiveExpression<DoubleExpression, f64> {
  public:
    static constexpr auto KIND = NodeKind::DOUBLE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(DoubleExpression)

    auto accept(Visitor& v) const -> void override;

    auto is_equal(const Node& other) const noexcept -> bool override;
};

class BoolExpression : public PrimitiveExpression<BoolExpression, bool> {
  public:
    static constexpr auto KIND = NodeKind::BOOL_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;
    MAKE_AST_COPY_MOVE(BoolExpression)

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    operator bool() const noexcept { return value_; }
};
template <> struct disable_default_parse<BoolExpression> : std::true_type {};

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace conch::ast

template <conch::ast::PrimitiveNode P> struct fmt::formatter<P> {
    static constexpr auto parse(format_parse_context& ctx) noexcept { return ctx.begin(); }

    template <typename F> static auto format(const P& p, F& ctx) {
        return fmt::format_to(ctx.out(), "{}", p.get_value());
    }
};
