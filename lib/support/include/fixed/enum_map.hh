#pragma once

#include <array>
#include <type_traits>

#include <magic_enum/magic_enum.hpp>

#include "assert.hh"
#include "enum.hh"
#include "iterator.hh"
#include "option.hh"
#include "type_traits.hh"

namespace porpoise::fixed {

// Empty or single-value enums aren't allowed since they eliminate the need for mapping
template <typename Enum>
concept MappableEnum = BoundedEnum<Enum> && magic_enum::enum_count<Enum>() > 1;

// An O(1) map that stores optional values for each enumeration
template <MappableEnum E, traits::TriviallyDestructible Value> class EnumMap {
  public:
    using Map = std::array<Value, magic_enum::enum_count<E>()>;
    MAKE_UNALIASED_ITERATOR(Map, map_)

  public:
    constexpr EnumMap() noexcept
        requires(std::is_default_constructible_v<Value>)
    = default;

    constexpr explicit EnumMap(Value default_value) noexcept { map_.fill(default_value); }

    // Asserts that the key is a valid enumeration
    [[nodiscard]] constexpr auto operator[](this auto&& self, E key) noexcept -> decltype(auto) {
        const auto index = magic_enum::enum_index(key);
        ASSERT(index, "Key must be a valid enumeration");
        return self.map_[*index];
    }

    // Returns the value at the key or none if contextually convertible
    //
    // Contextually convertible Values are pointers and optional types
    [[nodiscard]] constexpr auto get_opt(E key) const noexcept {
        if constexpr (traits::is_option_v<Value>) {
            return operator[](key);
        } else if constexpr (std::is_pointer_v<Value>) {
            const auto value = operator[](key);
            return value ? opt::Option<Value>{value} : opt::none;
        } else {
            return opt::Option<Value>{operator[](key)};
        }
    }

  private:
    Map map_{};
};

} // namespace porpoise::fixed
