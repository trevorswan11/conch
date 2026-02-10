#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "util/optional.hpp"

namespace conch {

using u8    = uint8_t;
using i8    = int8_t;
using u16   = uint16_t;
using i16   = int16_t;
using u32   = uint32_t;
using i32   = int32_t;
using u64   = uint64_t;
using i64   = int64_t;
using usize = size_t;
using isize = std::make_signed<usize>;

using f32 = float;
using f64 = double;

using byte = std::string_view::value_type;
static_assert(std::is_same_v<std::string::value_type, byte>);

template <typename T> using Box = std::unique_ptr<T>;
template <typename T, typename... Args> constexpr Box<T> make_box(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Makes a new box from an existing pointer
template <typename T, typename P> constexpr Box<T> box_from(P* ptr) {
    return std::unique_ptr<T>(static_cast<T*>(ptr));
}

// Makes a new box from an existing box, changing the type as requested
template <typename T, typename P> constexpr Box<T> box_into(Box<P>&& ptr) {
    return std::unique_ptr<T>(static_cast<T*>(ptr.release()));
}

template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr Rc<T> make_rc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

// Materializes a sized view into its corresponding array representation
template <auto N, typename Range> constexpr auto materialize_sized_view(Range&& r) {
    std::array<std::ranges::range_value_t<Range>, N> arr{};
    std::ranges::copy(r, arr.begin());
    return arr;
}

template <typename T, usize... Ns>
constexpr auto concat_arrays(const std::array<T, Ns>&... arrays) {
    std::array<T, (Ns + ...)> result{};
    usize                     offset{};
    ((std::ranges::copy(arrays, result.begin() + offset), offset += Ns), ...);
    return result;
}

template <typename... Args> auto todo_impl(Optional<std::string_view> message) noexcept -> void {
    if (message) { std::cerr << "TODO: " << *message << "\n"; }
    assert(false);
}

template <typename... Args> auto todo([[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(nullopt);
}

template <typename... Args>
auto todo(std::string_view message, [[maybe_unused]] Args&&... args) noexcept -> void {
    todo_impl(message);
}

#define TODO(...)      \
    todo(__VA_ARGS__); \
    std::unreachable()

} // namespace conch
