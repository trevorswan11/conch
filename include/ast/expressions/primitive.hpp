#pragma once

#include <string>
#include <utility>
#include <variant>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

template <typename T> class PrimitiveExpression : public Expression {
  public:
    explicit PrimitiveExpression(const Token& start_token, T value) noexcept
        : Expression{start_token}, value_{std::move(value)} {}

    auto value() const -> const T& { return value_; }

  private:
    T value_;
};

class StringExpression : public PrimitiveExpression<std::string> {
  public:
    using PrimitiveExpression<std::string>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<StringExpression>, ParserDiagnostic>;
};

class IntegerExpression : public PrimitiveExpression<usize> {
  public:
    using PrimitiveExpression<usize>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<IntegerExpression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<u8> {
  public:
    using PrimitiveExpression<u8>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ByteExpression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<f64> {
  public:
    using PrimitiveExpression<f64>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<FloatExpression>, ParserDiagnostic>;
};

class BoolExpression : public PrimitiveExpression<bool> {
  public:
    using PrimitiveExpression<bool>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<BoolExpression>, ParserDiagnostic>;
};

class VoidExpression : public PrimitiveExpression<std::monostate> {
  public:
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<VoidExpression>, ParserDiagnostic>;
};

class NilExpression : public PrimitiveExpression<std::monostate> {
  public:
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<NilExpression>, ParserDiagnostic>;
};

} // namespace ast
