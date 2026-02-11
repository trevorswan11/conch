#pragma once

#include <charconv>
#include <string>
#include <utility>
#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

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
        constexpr auto is_floating_point = std::is_same_v<value_type, f64>;
        const auto     start_token       = parser.current_token();
        const auto     base              = token_type::to_base(start_token.type);

        const auto* first = start_token.slice.cbegin() + (!base || *base == Base::DECIMAL ? 0 : 2);
        const auto* last  = start_token.slice.cend() - token_type::suffix_length(start_token.type);

        value_type             v;
        std::from_chars_result result;
        if constexpr (is_floating_point) {
            result = std::from_chars(first, last, v);
        } else {
            result = std::from_chars(first, last, v, std::to_underlying(*base));
        }
        if (result.ec == std::errc{} && result.ptr == last) {
            return make_box<Derived>(start_token, v);
        }

        if (result.ec == std::errc::result_out_of_range) {
            const auto err =
                is_floating_point ? ParserError::FLOAT_OVERFLOW : ParserError::INTEGER_OVERFLOW;
            return make_parser_unexpected(err, start_token);
        }

        const auto err =
            is_floating_point ? ParserError::MALFORMED_FLOAT : ParserError::MALFORMED_INTEGER;
        return make_parser_unexpected(err, start_token);
    }

    auto get_value() const -> const value_type& { return value_; }

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

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};
template <> struct disable_default_parse<StringExpression> : std::true_type {};

class SignedIntegerExpression : public PrimitiveExpression<SignedIntegerExpression, i32> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class SignedLongIntegerExpression : public PrimitiveExpression<SignedLongIntegerExpression, i64> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class ISizeIntegerExpression : public PrimitiveExpression<ISizeIntegerExpression, isize> {
  public:
    static constexpr auto KIND = NodeKind::ISIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class UnsignedIntegerExpression : public PrimitiveExpression<UnsignedIntegerExpression, u32> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class UnsignedLongIntegerExpression
    : public PrimitiveExpression<UnsignedLongIntegerExpression, u64> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class USizeIntegerExpression : public PrimitiveExpression<USizeIntegerExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::USIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;
};

class ByteExpression : public PrimitiveExpression<ByteExpression, byte> {
  public:
    static constexpr auto KIND = NodeKind::BYTE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};
template <> struct disable_default_parse<ByteExpression> : std::true_type {};

class FloatExpression : public PrimitiveExpression<FloatExpression, f64> {
  public:
    static constexpr auto KIND = NodeKind::FLOAT_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    static auto approx_eq(value_type a, value_type b) -> bool;
};

class BoolExpression : public PrimitiveExpression<BoolExpression, bool> {
  public:
    static constexpr auto KIND = NodeKind::BOOL_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    operator bool() const noexcept { return value_; }
};
template <> struct disable_default_parse<BoolExpression> : std::true_type {};

class NilExpression : public PrimitiveExpression<NilExpression, std::monostate> {
  public:
    static constexpr auto KIND = NodeKind::NIL_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};
template <> struct disable_default_parse<NilExpression> : std::true_type {};

// cppcheck-suppress-end [constParameterReference, duplInheritedMember]

} // namespace conch::ast
