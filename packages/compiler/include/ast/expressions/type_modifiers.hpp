#pragma once

#include <algorithm>
#include <utility>

#include "lexer/token.hpp"

#include "optional.hpp"
#include "types.hpp"

namespace conch::ast {

class TypeModifier {
  public:
    enum class Modifier : u8 {
        REF,
        REF_MUT,
    };

  public:
    TypeModifier() noexcept = default;
    explicit TypeModifier(Optional<Modifier> underlying) noexcept
        : underlying_{std::move(underlying)} {}

    static constexpr auto from_token(const Token& tok) -> TypeModifier {
        const auto it = std::ranges::find(LEGAL_MODIFIERS, tok.type, &ModifierMapping::first);
        return it == LEGAL_MODIFIERS.end() ? TypeModifier{nullopt} : TypeModifier{it->second};
    }

    // Whether or not the type is a 'value' type, mutually exclusive result.
    [[nodiscard]] auto is_value() const noexcept -> bool { return !underlying_; }

    // Whether or not the type is a const reference, mutually exclusive result.
    [[nodiscard]] auto is_const_ref() const noexcept -> bool {
        if (is_value()) { return false; }
        return *underlying_ == Modifier::REF;
    }

    // Whether or not the type is a mutable reference, mutually exclusive result.
    [[nodiscard]] auto is_mutable_ref() const noexcept -> bool {
        if (is_value()) { return false; }
        return *underlying_ == Modifier::REF_MUT;
    }

    friend auto operator==(const TypeModifier& lhs, const TypeModifier& rhs) noexcept -> bool {
        return lhs.underlying_ == rhs.underlying_;
    }

  private:
    using ModifierMapping                 = std::pair<TokenType, Modifier>;
    static constexpr auto LEGAL_MODIFIERS = std::to_array<ModifierMapping>({
        {TokenType::AND, Modifier::REF},
        {TokenType::AND_MUT, Modifier::REF_MUT},
    });

  private:
    Optional<Modifier> underlying_;
};

} // namespace conch::ast
