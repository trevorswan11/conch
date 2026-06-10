#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace ghoti::traits {

template <typename T>
concept Integral = std::is_integral_v<T> && !std::same_as<T, bool>;

template <typename T>
concept Signed = std::is_signed_v<T>;

template <typename T>
concept Unsigned = std::is_unsigned_v<T>;

template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template <typename T, typename... Args>
concept NoThrowConstructible = std::is_nothrow_constructible_v<T, Args...>;

template <typename T>
concept NoThrowMoveConstructible = std::is_nothrow_move_constructible_v<T>;

template <typename T>
concept NoThrowCopyConstructible = std::is_nothrow_copy_constructible_v<T>;

template <typename T>
concept TriviallyConstructible = std::is_trivially_constructible_v<T>;

template <typename T>
concept TriviallyDestructible = std::is_trivially_destructible_v<T>;

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

template <typename T, typename... Args>
concept Constructible = std::is_constructible_v<T, Args...>;

template <typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

template <typename T>
concept CopyConstructible = std::is_copy_constructible_v<T>;

template <typename T>
concept ScopedEnum = std::is_scoped_enum_v<T>;

template <typename T>
concept Reference = std::is_reference_v<T>;

template <typename T>
concept RValueReference = std::is_rvalue_reference_v<T>;

template <typename T>
concept Pointer = std::is_pointer_v<T>;

template <typename T>
concept Const = std::is_const_v<std::remove_reference_t<T>>;

// Returns `const T` if Self is const, `T` otherwise
template <typename Self, typename T>
using const_dispatch_t = std::conditional_t<Const<Self>, const T, T>;

} // namespace ghoti::traits
