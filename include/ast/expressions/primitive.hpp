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
    explicit PrimitiveExpression(const Token& start_token, T value) noexcept
        : Expression{start_token}, value_{std::move(value)} {}

    auto get_value() const -> const T& { return value_; }

  private:
    T value_;
};

class StringExpression : public PrimitiveExpression<std::string> {
  public:
    using PrimitiveExpression<std::string>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<StringExpression>, ParserDiagnostic>;
};

class IntegerExpression : public PrimitiveExpression<usize> {
  public:
    using PrimitiveExpression<usize>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<IntegerExpression>, ParserDiagnostic>;
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
