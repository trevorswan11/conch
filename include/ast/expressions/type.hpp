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

class ExplicitArrayType {
  public:
    explicit ExplicitArrayType(std::vector<usize>              dimensions,
                               std::unique_ptr<TypeExpression> inner_type) noexcept;
    ~ExplicitArrayType();

    ExplicitArrayType(const ExplicitArrayType&)                        = delete;
    auto operator=(const ExplicitArrayType&) -> ExplicitArrayType&     = delete;
    ExplicitArrayType(ExplicitArrayType&&) noexcept                    = default;
    auto operator=(ExplicitArrayType&&) noexcept -> ExplicitArrayType& = default;

    [[nodiscard]] auto dimensions() const noexcept -> const std::vector<usize>& {
        return dimensions_;
    }

    [[nodiscard]] auto inner_type() const noexcept -> const TypeExpression& { return *inner_type_; }

  private:
    std::vector<usize>              dimensions_;
    std::unique_ptr<TypeExpression> inner_type_;
};

enum class ExplicitTypeConstraint : u8 {
    PRIMITIVE,
    GENERIC_TYPE,
};

using ExplicitTypeVariant =
    std::variant<ExplicitIdentType, ExplicitReferredType, ExplicitFunctionType, ExplicitArrayType>;

class ExplicitType {
  public:
    explicit ExplicitType(ExplicitTypeVariant type, bool nullable) noexcept;
    explicit ExplicitType(ExplicitTypeVariant              type,
                          bool                             nullable,
                          Optional<ExplicitTypeConstraint> constraint) noexcept;
    ~ExplicitType();

    ExplicitType(const ExplicitType&)                        = delete;
    auto operator=(const ExplicitType&) -> ExplicitType&     = delete;
    ExplicitType(ExplicitType&&) noexcept                    = default;
    auto operator=(ExplicitType&&) noexcept -> ExplicitType& = default;

    [[nodiscard]] auto type() const noexcept -> const ExplicitTypeVariant& { return type_; }
    [[nodiscard]] auto nullable() const noexcept -> bool { return nullable_; }
    [[nodiscard]] auto constraint() const noexcept -> const Optional<ExplicitTypeConstraint>& {
        return constraint_;
    }

  private:
    ExplicitTypeVariant              type_;
    bool                             nullable_;
    Optional<ExplicitTypeConstraint> constraint_;
};

class TypeExpression : public Expression {
  public:
    explicit TypeExpression(const Token& start_token) noexcept;
    explicit TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::pair<std::unique_ptr<TypeExpression>, bool>, ParserDiagnostic>;

    [[nodiscard]] auto explicit_type() const noexcept -> const Optional<ExplicitType>& {
        return explicit_;
    }

  private:
    Optional<ExplicitType> explicit_;
};

} // namespace ast
