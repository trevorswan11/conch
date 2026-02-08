#pragma once

#include <string>
#include <utility>
#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

template <typename I, typename T> class PrimitiveExpression : public Expression {
  public:
    using value_type = T;

  public:
    PrimitiveExpression() = delete;

    auto get_value() const -> const value_type& { return value_; }

    auto is_equal(const Node& other) const noexcept -> bool override {
        const auto& casted = as<PrimitiveExpression>(other);
        return value_ == casted.value_;
    }

  protected:
    explicit PrimitiveExpression(const Token& start_token, value_type value) noexcept
        : Expression{start_token, I::KIND}, value_{std::move(value)} {}

  protected:
    value_type value_;
};

class StringExpression : public PrimitiveExpression<StringExpression, std::string> {
  public:
    static constexpr auto KIND = NodeKind::STRING_EXPRESSION;

  public:
    explicit StringExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class SignedIntegerExpression : public PrimitiveExpression<SignedIntegerExpression, i64> {
  public:
    static constexpr auto KIND = NodeKind::SIGNED_INTEGER_EXPRESSION;

  public:
    explicit SignedIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class UnsignedIntegerExpression : public PrimitiveExpression<UnsignedIntegerExpression, u64> {
  public:
    static constexpr auto KIND = NodeKind::UNSIGNED_INTEGER_EXPRESSION;

  public:
    explicit UnsignedIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class SizeIntegerExpression : public PrimitiveExpression<SizeIntegerExpression, usize> {
  public:
    static constexpr auto KIND = NodeKind::SIZE_INTEGER_EXPRESSION;

  public:
    explicit SizeIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<ByteExpression, byte> {
  public:
    static constexpr auto KIND = NodeKind::BYTE_EXPRESSION;

  public:
    explicit ByteExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<FloatExpression, f64> {
  public:
    static constexpr auto KIND = NodeKind::FLOAT_EXPRESSION;

  public:
    explicit FloatExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    static auto approx_eq(value_type a, value_type b) -> bool;
};

class BoolExpression : public PrimitiveExpression<BoolExpression, bool> {
  public:
    static constexpr auto KIND = NodeKind::BOOL_EXPRESSION;

  public:
    explicit BoolExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    operator bool() const noexcept { return value_; }
};

class NilExpression : public PrimitiveExpression<NilExpression, std::monostate> {
  public:
    static constexpr auto KIND = NodeKind::NIL_EXPRESSION;

  public:
    explicit NilExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;
};

} // namespace conch::ast
