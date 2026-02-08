#pragma once

#include <string>
#include <utility>
#include <variant>

#include "util/common.hpp"
#include "util/expected.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

template <typename T> class PrimitiveExpression : public Expression {
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
    explicit PrimitiveExpression(const Token& start_token, NodeKind kind, value_type value) noexcept
        : Expression{start_token, kind}, value_{std::move(value)} {}

  protected:
    value_type value_;
};

class StringExpression : public PrimitiveExpression<std::string> {
  public:
    explicit StringExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::STRING_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<StringExpression>, ParserDiagnostic>;
};

class SignedIntegerExpression : public PrimitiveExpression<i64> {
  public:
    explicit SignedIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::SIGNED_INTEGER_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<SignedIntegerExpression>, ParserDiagnostic>;
};

class UnsignedIntegerExpression : public PrimitiveExpression<u64> {
  public:
    explicit UnsignedIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{
              start_token, NodeKind::UNSIGNED_INTEGER_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<UnsignedIntegerExpression>, ParserDiagnostic>;
};

class SizeIntegerExpression : public PrimitiveExpression<usize> {
  public:
    explicit SizeIntegerExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::SIZE_INTEGER_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<SizeIntegerExpression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<byte> {
  public:
    explicit ByteExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::BYTE_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<ByteExpression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<f64> {
  public:
    explicit FloatExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::FLOAT_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<FloatExpression>, ParserDiagnostic>;

    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    static auto approx_eq(value_type a, value_type b) -> bool;
};

class BoolExpression : public PrimitiveExpression<bool> {
  public:
    explicit BoolExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::BOOL_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<BoolExpression>, ParserDiagnostic>;

    operator bool() const noexcept { return value_; }
};

class NilExpression : public PrimitiveExpression<std::monostate> {
  public:
    explicit NilExpression(const Token& start_token, value_type value) noexcept
        : PrimitiveExpression{start_token, NodeKind::NIL_EXPRESSION, std::move(value)} {}

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<NilExpression>, ParserDiagnostic>;
};

} // namespace conch::ast
