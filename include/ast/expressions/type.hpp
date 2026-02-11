#pragma once

#include <span>
#include <variant>
#include <vector>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;

using ExplicitIdentType    = Box<IdentifierExpression>;
using ExplicitReferredType = Box<Expression>;
using ExplicitFunctionType = Box<FunctionExpression>;

class ExplicitArrayType {
  public:
    explicit ExplicitArrayType(std::vector<usize>  dimensions,
                               Box<TypeExpression> inner_type) noexcept;
    ~ExplicitArrayType();

    ExplicitArrayType(const ExplicitArrayType&)                        = delete;
    auto operator=(const ExplicitArrayType&) -> ExplicitArrayType&     = delete;
    ExplicitArrayType(ExplicitArrayType&&) noexcept                    = default;
    auto operator=(ExplicitArrayType&&) noexcept -> ExplicitArrayType& = default;

    [[nodiscard]] auto get_dimensions() const noexcept -> std::span<const usize> {
        return dimensions_;
    }

    [[nodiscard]] auto get_inner_type() const noexcept -> const TypeExpression& {
        return *inner_type_;
    }

  private:
    std::vector<usize>  dimensions_;
    Box<TypeExpression> inner_type_;

    friend class ExplicitType;
    friend class TypeExpression;
};

enum class ExplicitTypeConstraint : u8 {
    PRIMITIVE,
    GENERIC_TYPE,
};

using ExplicitTypeVariant =
    std::variant<ExplicitIdentType, ExplicitReferredType, ExplicitFunctionType, ExplicitArrayType>;

class ExplicitType {
  public:
    explicit ExplicitType(ExplicitTypeVariant              type,
                          bool                             nullable,
                          Optional<ExplicitTypeConstraint> constraint) noexcept;
    ~ExplicitType();

    ExplicitType(const ExplicitType&)                        = delete;
    auto operator=(const ExplicitType&) -> ExplicitType&     = delete;
    ExplicitType(ExplicitType&&) noexcept                    = default;
    auto operator=(ExplicitType&&) noexcept -> ExplicitType& = default;

    [[nodiscard]] auto get_type() const noexcept -> const ExplicitTypeVariant& { return type_; }
    [[nodiscard]] auto is_nullable() const noexcept -> bool { return nullable_; }
    [[nodiscard]] auto has_constraint() const noexcept -> bool { return constraint_.has_value(); }
    [[nodiscard]] auto get_constraint() const noexcept -> const Optional<ExplicitTypeConstraint>& {
        return constraint_;
    }

  private:
    ExplicitTypeVariant              type_;
    bool                             nullable_;
    Optional<ExplicitTypeConstraint> constraint_;

    friend class TypeExpression;
};

class TypeExpression : public ExprBase<TypeExpression> {
  public:
    static constexpr auto KIND = NodeKind::TYPE_EXPRESSION;

  public:
    explicit TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<std::pair<Box<Expression>, bool>, ParserDiagnostic>;

    [[nodiscard]] auto has_explicit_type() const noexcept -> bool { return explicit_.has_value(); }
    [[nodiscard]] auto get_explicit_type() const noexcept -> const Optional<ExplicitType>& {
        return explicit_;
    }

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    Optional<ExplicitType> explicit_;
};

} // namespace conch::ast
