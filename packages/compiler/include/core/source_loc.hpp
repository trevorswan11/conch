#pragma once

#include <concepts>

#include "core/common.hpp"

namespace conch {

struct SourceLocation {
    usize line   = 0;
    usize column = 0;

    SourceLocation(usize line, usize column) : line{line}, column{column} {}

    auto operator==(const SourceLocation& other) const noexcept -> bool {
        return line == other.line && column == other.column;
    }
};

template <typename T> struct SourceInfo;

template <typename T>
concept Locateable = requires(T t) {
    { SourceInfo<T>::get(t) } -> std::same_as<SourceLocation>;
};

} // namespace conch
