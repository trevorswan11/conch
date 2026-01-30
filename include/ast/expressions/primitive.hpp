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
    explicit PrimitiveExpression(const Token& start_token, value_type value) noexcept
        : Expression{start_token}, value_{std::move(value)} {}

    auto get_value() const -> const value_type& { return value_; }

  private:
    value_type value_;
};

class StringExpression : public PrimitiveExpression<std::string> {
  public:
    using PrimitiveExpression<std::string>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<StringExpression>, ParserDiagnostic>;
};

class SignedIntegerExpression : public PrimitiveExpression<i64> {
  public:
    using PrimitiveExpression<i64>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<SignedIntegerExpression>, ParserDiagnostic>;
};

class UnsignedIntegerExpression : public PrimitiveExpression<u64> {
  public:
    using PrimitiveExpression<u64>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<UnsignedIntegerExpression>, ParserDiagnostic>;
};

class SizeIntegerExpression : public PrimitiveExpression<usize> {
  public:
    using PrimitiveExpression<usize>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<SizeIntegerExpression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<u8> {
  public:
    using PrimitiveExpression<u8>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<ByteExpression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<f64> {
  public:
    using PrimitiveExpression<f64>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<FloatExpression>, ParserDiagnostic>;
};

class BoolExpression : public PrimitiveExpression<bool> {
  public:
    using PrimitiveExpression<bool>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<BoolExpression>, ParserDiagnostic>;
};

class VoidExpression : public PrimitiveExpression<std::monostate> {
  public:
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<VoidExpression>, ParserDiagnostic>;
};

class NilExpression : public PrimitiveExpression<std::monostate> {
  public:
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<NilExpression>, ParserDiagnostic>;
};

} // namespace conch::ast
