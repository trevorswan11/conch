#pragma once

#include <utility>

#include "ast/expressions/type_modifiers.hpp"
#include "ast/node.hpp"

#include "parser/parser.hpp"

#include "variant.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;
class USizeIntegerExpression;

class ExplicitType;

class ExplicitArrayType {
  public:
    explicit ExplicitArrayType(Box<Expression> dimension, Box<ExplicitType> inner_type) noexcept;
    ~ExplicitArrayType();

    MAKE_AST_COPY_MOVE(ExplicitArrayType)

    [[nodiscard]] auto get_dimensions() const noexcept -> const Expression& { return *dimension_; }
    [[nodiscard]] auto get_inner_type() const noexcept -> const ExplicitType& {
        return *inner_type_;
    }

  private:
    Box<Expression>   dimension_;
    Box<ExplicitType> inner_type_;

    friend class ExplicitType;
    friend class TypeExpression;
};

class ExplicitType {
  public:
    using ExplicitIdentType     = Box<IdentifierExpression>;
    using ExplicitFunctionType  = Box<FunctionExpression>;
    using ExplicitRecursiveType = Box<ExplicitType>;

    using ExplicitTypeVariant = std::
        variant<ExplicitIdentType, ExplicitFunctionType, ExplicitArrayType, ExplicitRecursiveType>;

  public:
    explicit ExplicitType(TypeModifier modifier, ExplicitTypeVariant type) noexcept;
    ~ExplicitType();

    MAKE_AST_COPY_MOVE(ExplicitType)

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<ExplicitType, ParserDiagnostic>;

    [[nodiscard]] auto get_modifier() const noexcept -> const TypeModifier& { return modifier_; }
    [[nodiscard]] auto get_type() const noexcept -> const ExplicitTypeVariant& { return type_; }

    MAKE_VARIANT_UNPACKER(ident_type, IdentifierExpression, ExplicitIdentType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(function_type, FunctionExpression, ExplicitFunctionType, type_, *std::get)
    MAKE_VARIANT_UNPACKER(array_type, ExplicitArrayType, ExplicitArrayType, type_, std::get)
    MAKE_VARIANT_UNPACKER(
        recursive_type, ExplicitRecursiveType, ExplicitRecursiveType, type_, std::get)

    friend auto operator==(const ExplicitType& lhs, const ExplicitType& rhs) noexcept -> bool {
        return lhs.is_equal(rhs);
    }

  private:
    auto is_equal(const ExplicitType& other) const noexcept -> bool;

  private:
    TypeModifier        modifier_;
    ExplicitTypeVariant type_;

    friend class TypeExpression;
};

class TypeExpression : public ExprBase<TypeExpression> {
  public:
    static constexpr auto KIND = NodeKind::TYPE_EXPRESSION;

  public:
    explicit TypeExpression(const Token& start_token, Optional<ExplicitType> exp) noexcept;
    ~TypeExpression() override;

    MAKE_AST_COPY_MOVE(TypeExpression)

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
