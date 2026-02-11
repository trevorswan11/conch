#pragma once

#include <string>
#include <utility>
#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

// A necessarily instantiable Node with an underlying primitive value type.
template <typename N>
concept PrimitiveNode = LeafNode<N> && requires { typename N::value_type; };

template <typename Derived, typename T> class PrimitiveExpression : public ExprBase<Derived> {
  public:
    using value_type = T;

  public:
    explicit PrimitiveExpression(const Token& start_token, value_type value) noexcept
        : ExprBase<Derived>{start_token}, value_{std::move(value)} {}

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

class SignedIntegerExpression : public PrimitiveExpression<SignedIntegerExpression, i32> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class SignedLongIntegerExpression : public PrimitiveExpression<SignedLongIntegerExpression, i64> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class ISizeIntegerExpression : public PrimitiveExpression<ISizeIntegerExpression, isize> {
  public:
    static constexpr auto KIND = NodeKind::ISIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class UnsignedIntegerExpression : public PrimitiveExpression<UnsignedIntegerExpression, u32> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class UnsignedLongIntegerExpression
    : public PrimitiveExpression<UnsignedLongIntegerExpression, u64> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_LONG_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class USizeIntegerExpression : public PrimitiveExpression<USizeIntegerExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::USIZE_INTEGER_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<ByteExpression, byte> {
  public:
    static constexpr auto KIND = NodeKind::BYTE_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<FloatExpression, f64> {
  public:
    static constexpr auto KIND = NodeKind::FLOAT_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

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

class NilExpression : public PrimitiveExpression<NilExpression, std::monostate> {
  public:
    static constexpr auto KIND = NodeKind::NIL_EXPRESSION;

  public:
    using PrimitiveExpression::PrimitiveExpression;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

} // namespace conch::ast
