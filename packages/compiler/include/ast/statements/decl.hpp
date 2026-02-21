#pragma once

#include <algorithm>
#include <array>
#include <bit>

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;

enum class DeclModifiers : u8 {
    VARIABLE = 1 << 0,
    CONSTANT = 1 << 1,
    COMPTIME = 1 << 2,
    PRIVATE  = 1 << 3,
    EXTERN   = 1 << 4,
    EXPORT   = 1 << 5,
    STATIC   = 1 << 6,
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

class DeclStatement : public StmtBase<DeclStatement> {
  public:
    static constexpr auto KIND = NodeKind::DECL_STATEMENT;

  public:
    explicit DeclStatement(const Token&              start_token,
                           Box<IdentifierExpression> ident,
                           Box<TypeExpression>       type,
                           Optional<Box<Expression>> value,
                           DeclModifiers             modifiers) noexcept;
    ~DeclStatement() override;

    auto                      accept(Visitor& v) const -> void override;
    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Statement>, ParserDiagnostic>;

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

  protected:
    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    using ModifierMapping                 = std::pair<TokenType, DeclModifiers>;
    static constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
        {TokenType::VAR, DeclModifiers::VARIABLE},
        {TokenType::CONST, DeclModifiers::CONSTANT},
        {TokenType::COMPTIME, DeclModifiers::COMPTIME},
        {TokenType::PRIVATE, DeclModifiers::PRIVATE},
        {TokenType::EXTERN, DeclModifiers::EXTERN},
        {TokenType::EXPORT, DeclModifiers::EXPORT},
        {TokenType::STATIC, DeclModifiers::STATIC},
    });

    static constexpr auto validate_modifiers(DeclModifiers modifiers) noexcept -> bool {
        // Exactly one mutability flag must be set
        const auto valid_mut = std::popcount(std::to_underlying(
                                   modifiers & (DeclModifiers::VARIABLE | DeclModifiers::CONSTANT |
                                                DeclModifiers::COMPTIME))) == 1;

        // Comptime values cannot be known at link time, obviously
        const auto valid_comptime =
            std::popcount(std::to_underlying(
                modifiers & (DeclModifiers::EXTERN | DeclModifiers::COMPTIME))) <= 1;

        // At most one ABI flag can be set
        const auto valid_abi =
            std::popcount(std::to_underlying(modifiers &
                                             (DeclModifiers::EXTERN | DeclModifiers::EXPORT))) <= 1;

        // At most one access flag can be set
        const auto valid_access = std::popcount(std::to_underlying(
                                      modifiers & (DeclModifiers::PRIVATE | DeclModifiers::EXTERN |
                                                   DeclModifiers::EXPORT))) <= 1;
        return valid_mut && valid_comptime && valid_abi && valid_access;
    }

    static constexpr auto token_to_modifier(const Token& tok) -> Optional<DeclModifiers> {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? nullopt : Optional<DeclModifiers>{it->second};
    }

  private:
    Box<IdentifierExpression> ident_;
    Box<TypeExpression>       type_;
    Optional<Box<Expression>> value_;
    DeclModifiers             modifiers_;
};

} // namespace conch::ast
