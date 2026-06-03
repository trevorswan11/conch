#pragma once

#include <bit>
#include <limits>
#include <string_view>

#include <fmt/base.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include "ast/kind.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include "assert.hh"
#include "option.hh"
#include "types.hh"

namespace ghoti {

namespace ast {

namespace detail { constexpr u64 INVALID_ID = std::numeric_limits<u64>::max(); } // namespace detail

// A compact id for all AST nodes
class NodeID {
  public:
    constexpr NodeID(NodeKind kind, syntax::TokenType type, u64 index) noexcept : raw_{} {
        ASSERT(index <= INDEX_MASK, "Requested node index is too large");
        raw_ |= static_cast<u64>(kind) << KIND_OFFSET;
        raw_ |= static_cast<u64>(type) << TOKEN_TYPE_OFFSET;
        raw_ |= index;
    }

    [[nodiscard]] constexpr auto get_kind() const noexcept -> NodeKind {
        return static_cast<NodeKind>((raw_ & KIND_MASK) >> KIND_OFFSET);
    }

    // Useful when the token type holds information about a node that is not known at creation
    constexpr auto set_token_type(syntax::TokenType type) noexcept -> void {
        raw_ &= ~TOKEN_TYPE_MASK;
        raw_ |= static_cast<u64>(type) << TOKEN_TYPE_OFFSET;
    }

    [[nodiscard]] constexpr auto get_token_type() const noexcept -> syntax::TokenType {
        return static_cast<syntax::TokenType>((raw_ & TOKEN_TYPE_MASK) >> TOKEN_TYPE_OFFSET);
    }

    [[nodiscard]] constexpr auto get_index() const noexcept -> usize {
        return static_cast<usize>(raw_ & INDEX_MASK);
    }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> NodeID {
        return NodeID{detail::INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
        return raw_ != detail::INVALID_ID;
    }

    template <traits::ASTNode N> [[nodiscard]] constexpr auto is() const noexcept -> bool {
        return get_kind() == traits::NodeKindOf<N>::value();
    }

    template <traits::ASTNode... Ns> [[nodiscard]] constexpr auto any() const noexcept -> bool {
        const auto kind = get_kind();
        return ((kind == traits::NodeKindOf<Ns>::value()) || ...);
    }

    [[nodiscard]] auto display_name() const noexcept -> std::string_view;

  private:
    constexpr explicit NodeID(u64 raw) noexcept : raw_{raw} {}

  private:
    static constexpr u64 KIND_MASK         = 0xFF00000000000000ULL;
    static constexpr u64 KIND_OFFSET       = std::countr_zero(KIND_MASK);
    static constexpr u64 TOKEN_TYPE_MASK   = 0x00FF000000000000ULL;
    static constexpr u64 TOKEN_TYPE_OFFSET = std::countr_zero(TOKEN_TYPE_MASK);
    static constexpr u64 INDEX_MASK        = 0x0000FFFFFFFFFFFFULL;

  private:
    u64 raw_;

    template <NodeKind... AllowedKinds> friend class Handle;
};

} // namespace ast

namespace traits {

template <> struct Nullable<ast::NodeID> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::NodeID {
        return ast::NodeID::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(ast::NodeID id) noexcept -> bool {
        return id.is_valid();
    }
};

} // namespace traits

namespace ast {

#define MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(name, modifier)            \
    [[nodiscard]] constexpr auto is_##name() const noexcept -> bool { \
        if (is_value()) { return false; }                             \
        return underlying_ == modifier;                               \
    }

class TypeModifier {
  public:
    enum class Modifier : u8 {
        VALUE,
        REF,
        MUT_REF,
        PTR,
        MUT_PTR,
        VOLATILE,
        MUT_VOLATILE,
    };

  public:
    constexpr TypeModifier() noexcept = default;
    constexpr explicit TypeModifier(Modifier underlying) noexcept : underlying_{underlying} {}
    constexpr explicit TypeModifier(u64 raw) noexcept : underlying_{static_cast<Modifier>(raw)} {}
    explicit TypeModifier(const syntax::Token& tok) noexcept;

    [[nodiscard]] constexpr auto get_raw() const noexcept -> Modifier { return underlying_; }

    // Whether or not the type is a 'value' type (no modifier), mutually exclusive result.
    [[nodiscard]] constexpr auto is_value() const noexcept -> bool {
        return underlying_ == Modifier::VALUE;
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ref, Modifier::MUT_REF)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ref, Modifier::REF)
    [[nodiscard]] constexpr auto is_ref() const noexcept -> bool {
        return is_mutable_ref() || is_const_ref();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_ptr, Modifier::MUT_PTR)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_ptr, Modifier::PTR)
    [[nodiscard]] constexpr auto is_ptr() const noexcept -> bool {
        return is_mutable_ptr() || is_const_ptr();
    }

    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(mutable_volatile, Modifier::MUT_VOLATILE)
    MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY(const_volatile, Modifier::VOLATILE)
    [[nodiscard]] constexpr auto is_volatile() const noexcept -> bool {
        return is_mutable_volatile() || is_const_volatile();
    }

    [[nodiscard]] constexpr auto operator==(const TypeModifier& other) const noexcept
        -> bool = default;

    [[nodiscard]] constexpr explicit operator u64() const noexcept {
        return static_cast<u64>(underlying_);
    }

  private:
    Modifier underlying_{Modifier::VALUE};

    friend struct fmt::formatter<ghoti::ast::TypeModifier>;
};

// A compact id for all AST explicit types
class ExplicitTypeID {
  public:
    constexpr ExplicitTypeID(ExplicitTypeKind  kind,
                             TypeModifier      mod,
                             syntax::TokenType token_type,
                             u64               index) noexcept
        : raw_{} {
        static_assert(magic_enum::enum_count<ExplicitTypeKind>() <= 0xF,
                      "ExplicitTypeKind enum too large");
        static_assert(magic_enum::enum_count<TypeModifier::Modifier>() <= 0xF,
                      "TypeModifier enum too large");

        ASSERT(index <= INDEX_MASK, "Requested type index is too large");
        raw_ |= static_cast<u64>(kind) << KIND_OFFSET;
        raw_ |= static_cast<u64>(mod) << MODIFIER_OFFSET;
        raw_ |= static_cast<u64>(token_type) << TOKEN_TYPE_OFFSET;
        raw_ |= index;
    }

    [[nodiscard]] constexpr auto get_kind() const noexcept -> ExplicitTypeKind {
        return static_cast<ExplicitTypeKind>((raw_ & KIND_MASK) >> KIND_OFFSET);
    }

    [[nodiscard]] constexpr auto get_modifier() const noexcept -> TypeModifier {
        return TypeModifier{(raw_ & MODIFIER_MASK) >> MODIFIER_OFFSET};
    }

    [[nodiscard]] constexpr auto get_token_type() const noexcept -> syntax::TokenType {
        return static_cast<syntax::TokenType>((raw_ & TOKEN_TYPE_MASK) >> TOKEN_TYPE_OFFSET);
    }

    [[nodiscard]] constexpr auto get_index() const noexcept -> usize {
        return static_cast<usize>(raw_ & INDEX_MASK);
    }

    [[nodiscard]] static constexpr auto make_invalid() noexcept -> ExplicitTypeID {
        return ExplicitTypeID{detail::INVALID_ID};
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool {
        return raw_ != detail::INVALID_ID;
    }

    template <traits::ASTExplicitType N> [[nodiscard]] constexpr auto is() const noexcept -> bool {
        return get_kind() == traits::ExplicitTypeKindOf<N>::value();
    }

    template <traits::ASTExplicitType... Ns>
    [[nodiscard]] constexpr auto any() const noexcept -> bool {
        const auto kind = get_kind();
        return ((kind == traits::ExplicitTypeKindOf<Ns>::value()) || ...);
    }

  private:
    constexpr explicit ExplicitTypeID(const u64& raw) noexcept : raw_{raw} {}

  private:
    static constexpr u64 KIND_MASK         = 0xF000000000000000ULL;
    static constexpr u64 KIND_OFFSET       = std::countr_zero(KIND_MASK);
    static constexpr u64 MODIFIER_MASK     = 0x0F00000000000000ULL;
    static constexpr u64 MODIFIER_OFFSET   = std::countr_zero(MODIFIER_MASK);
    static constexpr u64 TOKEN_TYPE_MASK   = 0x00FF000000000000ULL;
    static constexpr u64 TOKEN_TYPE_OFFSET = std::countr_zero(TOKEN_TYPE_MASK);
    static constexpr u64 INDEX_MASK        = 0x0000FFFFFFFFFFFFULL;

  private:
    u64 raw_;
};

#undef MAKE_MUTUALLY_EXCLUSIVE_TYPE_QUERY

} // namespace ast

namespace traits {

template <> struct Nullable<ast::ExplicitTypeID> {
    [[nodiscard]] static constexpr auto invalid() noexcept -> ast::ExplicitTypeID {
        return ast::ExplicitTypeID::make_invalid();
    }

    [[nodiscard]] static constexpr auto is_valid(ast::ExplicitTypeID id) noexcept -> bool {
        return id.is_valid();
    }
};

} // namespace traits

} // namespace ghoti
