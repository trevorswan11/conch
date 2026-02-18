#pragma once

#include <type_traits>
#include <string_view>

namespace conch {

using u8    = std::uint8_t;
using i8    = std::int8_t;
using u16   = std::uint16_t;
using i16   = std::int16_t;
using u32   = std::uint32_t;
using i32   = std::int32_t;
using u64   = std::uint64_t;
using i64   = std::int64_t;
using usize = std::size_t;
using isize = std::make_signed_t<usize>;

using f32 = float;
using f64 = double;

using byte = std::string_view::value_type;

} // namespace conch
