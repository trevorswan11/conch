#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>

namespace porpoise::traits {

template <typename T>
concept Integral = std::is_integral_v<T> && !std::same_as<T, bool>;

template <typename T>
concept Unsigned = std::is_unsigned_v<T>;

template <typename T>
concept TriviallyConstructible = std::is_trivially_constructible_v<T>;

template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T>
concept ScopedEnum = std::is_scoped_enum_v<T>;

template <typename T>
concept Reference = std::is_reference_v<T>;

template <typename T> constexpr auto is_const_v = std::is_const_v<std::remove_reference_t<T>>;

// Returns `const T` if Self is const, `T` otherwise
template <typename Self, typename T>
using const_dispatch_t = std::conditional_t<is_const_v<Self>, const T, T>;

} // namespace porpoise::traits
