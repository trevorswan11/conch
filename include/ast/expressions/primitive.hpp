#pragma once

#include <cstdint>
#include <string>
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
    using PrimitiveExpression<std::string>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<StringExpression>, ParserDiagnostic>;
};

class SignedIntegerExpression : public PrimitiveExpression<int64_t> {
    using PrimitiveExpression<int64_t>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<SignedIntegerExpression>, ParserDiagnostic>;
};

class UnsignedIntegerExpression : public PrimitiveExpression<uint64_t> {
    using PrimitiveExpression<uint64_t>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<UnsignedIntegerExpression>, ParserDiagnostic>;
};

class SizeIntegerExpression : public PrimitiveExpression<size_t> {
    using PrimitiveExpression<size_t>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<SizeIntegerExpression>, ParserDiagnostic>;
};

class ByteExpression : public PrimitiveExpression<uint8_t> {
    using PrimitiveExpression<uint8_t>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<ByteExpression>, ParserDiagnostic>;
};

class BoolExpression : public PrimitiveExpression<bool> {
    using PrimitiveExpression<bool>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<BoolExpression>, ParserDiagnostic>;
};

class FloatExpression : public PrimitiveExpression<double> {
    using PrimitiveExpression<double>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<FloatExpression>, ParserDiagnostic>;
};

class VoidExpression : public PrimitiveExpression<std::monostate> {
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<VoidExpression>, ParserDiagnostic>;
};

class NilExpression : public PrimitiveExpression<std::monostate> {
    using PrimitiveExpression<std::monostate>::PrimitiveExpression;

    auto accept(Visitor& v) -> void override;

    static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<NilExpression>, ParserDiagnostic>;
};

} // namespace ast
