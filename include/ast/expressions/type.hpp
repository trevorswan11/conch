#pragma once

#include <memory>
#include <variant>
#include <vector>

#include "core.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;

using ExplicitIdentType    = std::unique_ptr<IdentifierExpression>;
using ExplicitReferredType = std::unique_ptr<Expression>;
using ExplicitFunctionType = std::unique_ptr<FunctionExpression>;

struct ExplicitArrayType {
    explicit ExplicitArrayType(std::vector<usize> d, std::unique_ptr<TypeExpression> i) noexcept;
    ~ExplicitArrayType();

    ExplicitArrayType(const ExplicitArrayType&)                        = delete;
    auto operator=(const ExplicitArrayType&) -> ExplicitArrayType&     = delete;
    ExplicitArrayType(ExplicitArrayType&&) noexcept                    = default;
    auto operator=(ExplicitArrayType&&) noexcept -> ExplicitArrayType& = default;

    std::vector<usize>              dimensions;
    std::unique_ptr<TypeExpression> inner_type;
};

enum class ExplicitTypeConstraint : u8 {
    PRIMITIVE,
    GENERIC_TYPE,
};

using ExplicitTypeVariant =
    std::variant<ExplicitIdentType, ExplicitReferredType, ExplicitFunctionType, ExplicitArrayType>;

struct ExplicitType {
    explicit ExplicitType(ExplicitTypeVariant t, bool n) noexcept;
    explicit ExplicitType(ExplicitTypeVariant              t,
                          bool                             n,
                          Optional<ExplicitTypeConstraint> c) noexcept;
    ~ExplicitType();

    ExplicitType(const ExplicitType&)                        = delete;
    auto operator=(const ExplicitType&) -> ExplicitType&     = delete;
    ExplicitType(ExplicitType&&) noexcept                    = default;
    auto operator=(ExplicitType&&) noexcept -> ExplicitType& = default;

    ExplicitTypeVariant              type;
    bool                             nullable;
    Optional<ExplicitTypeConstraint> constraint;
};

class TypeExpression : public Expression {
  public:
    explicit TypeExpression(const Token& start_token) noexcept;
    explicit TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::unique_ptr<TypeExpression>, ParserDiagnostic>;

    [[nodiscard]] auto explicit_type() const noexcept -> const Optional<ExplicitType>& {
        return explicit_;
    }

  private:
    Optional<ExplicitType> explicit_;
};

} // namespace ast
