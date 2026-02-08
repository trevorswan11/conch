#pragma once

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

template <typename T> using Rc = std::shared_ptr<T>;
template <typename T, typename... Args> constexpr Rc<T> make_rc(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <class... Ts> struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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
