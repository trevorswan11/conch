#pragma once

// 128-bit integers back compile-time evaluation and wide integer literals (D4).
// GNU extension; the whole project builds with clang, which supports it.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic"
using i128 = __int128;
using u128 = unsigned __int128;
#pragma clang diagnostic pop
