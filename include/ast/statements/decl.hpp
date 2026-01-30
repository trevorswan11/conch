#pragma once

#include <algorithm>
#include <array>
#include <utility>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;

enum class DeclModifiers : u8 {
    MUTABLE  = 1 << 0,
    CONSTANT = 1 << 1,
    PRIVATE  = 1 << 2,
    EXTERN   = 1 << 3,
    EXPORT   = 1 << 4,
};

constexpr auto operator|(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr auto operator&(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

constexpr auto operator^(DeclModifiers lhs, DeclModifiers rhs) -> DeclModifiers {
    return static_cast<DeclModifiers>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

constexpr auto operator|=(DeclModifiers& lhs, DeclModifiers rhs) -> DeclModifiers& {
    lhs = lhs | rhs;
    return lhs;
}

class DeclStatement : public Statement {
  public:
    explicit DeclStatement(const Token&              start_token,
                           Box<IdentifierExpression> ident,
                           Box<TypeExpression>       type,
                           Optional<Box<Expression>> value,
                           DeclModifiers             modifiers) noexcept;
    ~DeclStatement() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser)
        -> Expected<Box<DeclStatement>, ParserDiagnostic>;

    [[nodiscard]] auto get_ident() const noexcept -> const IdentifierExpression& { return *ident_; }
    [[nodiscard]] auto get_type() const noexcept -> const TypeExpression& { return *type_; }
    [[nodiscard]] auto has_value() const noexcept -> bool { return value_.has_value(); }
    [[nodiscard]] auto get_value() const noexcept -> Optional<const Expression&> {
        return value_ ? Optional<const Expression&>{**value_} : nullopt;
    }

    [[nodiscard]] auto get_modifiers() const noexcept -> const DeclModifiers& { return modifiers_; }
    [[nodiscard]] auto has_modifier(DeclModifiers flag) const noexcept -> bool {
        return static_cast<bool>(modifiers_ & flag);
    }

    static auto modifiers_has(DeclModifiers modifiers, DeclModifiers flag) noexcept -> bool {
        return static_cast<bool>(modifiers & flag);
    }

    static constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept -> bool {
        const auto unique_mut =
            (modifiers & DeclModifiers::MUTABLE) ^ (modifiers & DeclModifiers::CONSTANT);
        const auto unique_abi =
            (modifiers & DeclModifiers::EXTERN) ^ (modifiers & DeclModifiers::EXPORT);
        const auto unique_access =
            (modifiers & DeclModifiers::PRIVATE) ^ (modifiers & DeclModifiers::EXTERN);

        return static_cast<bool>(unique_mut & unique_abi & unique_access);
    }

    static constexpr auto token_to_modifier(const Token& tok) -> Optional<DeclModifiers> {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? nullopt : Optional<DeclModifiers>{it->second};
    }

  private:
    using ModifierMapping                 = std::pair<TokenType, DeclModifiers>;
    static constexpr auto LEGAL_MODIFIERS = std::array{
        ModifierMapping{TokenType::VAR, DeclModifiers::MUTABLE},
        ModifierMapping{TokenType::CONST, DeclModifiers::CONSTANT},
        ModifierMapping{TokenType::PRIVATE, DeclModifiers::PRIVATE},
        ModifierMapping{TokenType::EXTERN, DeclModifiers::EXTERN},
        ModifierMapping{TokenType::EXPORT, DeclModifiers::EXPORT},
    };

  private:
    Box<IdentifierExpression> ident_;
    Box<TypeExpression>       type_;
    Optional<Box<Expression>> value_;
    DeclModifiers             modifiers_;
};

} // namespace conch::ast
