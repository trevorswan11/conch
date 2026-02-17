#pragma once

#include <array>
#include <bit>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "core/common.hpp"
#include "core/expected.hpp"
#include "core/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;
class USizeIntegerExpression;

using ExplicitIdentType    = Box<IdentifierExpression>;
using ExplicitFunctionType = Box<FunctionExpression>;

class ExplicitArrayType {
  public:
    explicit ExplicitArrayType(std::vector<Box<USizeIntegerExpression>> dimensions,
                               Box<TypeExpression>                      inner_type) noexcept;
    ~ExplicitArrayType();

    ExplicitArrayType(const ExplicitArrayType&)                        = delete;
    auto operator=(const ExplicitArrayType&) -> ExplicitArrayType&     = delete;
    ExplicitArrayType(ExplicitArrayType&&) noexcept                    = default;
    auto operator=(ExplicitArrayType&&) noexcept -> ExplicitArrayType& = default;

    [[nodiscard]] auto get_dimensions() const noexcept
        -> std::span<const Box<USizeIntegerExpression>> {
        return dimensions_;
    }

    [[nodiscard]] auto get_inner_type() const noexcept -> const TypeExpression& {
        return *inner_type_;
    }

  private:
    std::vector<Box<USizeIntegerExpression>> dimensions_;
    Box<TypeExpression>                      inner_type_;

    friend class ExplicitType;
    friend class TypeExpression;
};

using ExplicitTypeVariant =
    std::variant<ExplicitIdentType, ExplicitFunctionType, ExplicitArrayType>;

enum class TypeModifiers : u8 {
    MUT = 1 << 1,
    REF = 1 << 2,
};

constexpr auto operator|=(TypeModifiers& lhs, TypeModifiers rhs) -> TypeModifiers& {
    lhs = static_cast<TypeModifiers>(std::to_underlying(lhs) | std::to_underlying(rhs));
    return lhs;
}

class ExplicitType {
  public:
    explicit ExplicitType(const Optional<TypeModifiers>& modifiers,
                          ExplicitTypeVariant            type,
                          bool                           primitive) noexcept;
    ~ExplicitType();

    ExplicitType(const ExplicitType&)                        = delete;
    auto operator=(const ExplicitType&) -> ExplicitType&     = delete;
    ExplicitType(ExplicitType&&) noexcept                    = default;
    auto operator=(ExplicitType&&) noexcept -> ExplicitType& = default;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<ExplicitType, ParserDiagnostic>;

    [[nodiscard]] auto get_type() const noexcept -> const ExplicitTypeVariant& { return type_; }
    [[nodiscard]] auto is_primitive() const noexcept -> bool { return primitive_; }

    // UB if the import is not a module import.
    [[nodiscard]] auto get_ident_type() const noexcept -> const IdentifierExpression& {
        try {
            return *std::get<ExplicitIdentType>(type_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_mident_type() const noexcept -> bool {
        return std::holds_alternative<ExplicitIdentType>(type_);
    }

    // UB if the import is not a function type.
    [[nodiscard]] auto get_function_type() const noexcept -> const FunctionExpression& {
        try {
            return *std::get<ExplicitFunctionType>(type_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_function_type() const noexcept -> bool {
        return std::holds_alternative<ExplicitFunctionType>(type_);
    }

    // UB if the import is not a array type.
    [[nodiscard]] auto get_array_type() const noexcept -> const ExplicitArrayType& {
        try {
            return std::get<ExplicitArrayType>(type_);
        } catch (...) { std::unreachable(); }
    }

    [[nodiscard]] auto is_array_type() const noexcept -> bool {
        return std::holds_alternative<ExplicitArrayType>(type_);
    }

    // Whether or not the type is a 'value' type, mutually exclusive result.
    [[nodiscard]] auto is_value() const noexcept -> bool { return !modifiers_.has_value(); }

    // Whether or not the type is a mutable reference, mutually exclusive result.
    [[nodiscard]] auto is_mutable() const noexcept -> bool {
        if (is_value()) { return false; }
        const auto bits = std::to_underlying(*modifiers_);
        return bits & (std::to_underlying(TypeModifiers::MUT));
    }

    // Whether or not the type is a const reference, mutually exclusive result.
    [[nodiscard]] auto is_reference() const noexcept -> bool {
        if (is_value()) { return false; }
        const auto bits = std::to_underlying(*modifiers_);
        return bits & (std::to_underlying(TypeModifiers::REF));
    }

  private:
    using ModifierMapping                 = std::pair<TokenType, TypeModifiers>;
    static constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
        {TokenType::MUT, TypeModifiers::MUT},
        {TokenType::REF, TypeModifiers::REF},
    });

    // Only valid if exactly one modifier is set
    static constexpr auto validate_modifiers(TypeModifiers modifiers) noexcept -> bool {
        const auto bits = std::to_underlying(modifiers);
        return std::popcount(bits) == 1;
    }

    static constexpr auto token_to_modifier(const Token& tok) -> Optional<TypeModifiers> {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? nullopt : Optional<TypeModifiers>{it->second};
    }

  private:
    Optional<TypeModifiers> modifiers_;
    ExplicitTypeVariant     type_;
    bool                    primitive_;

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
