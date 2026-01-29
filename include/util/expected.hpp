#pragma once

#include <expected>

namespace conch {

template <typename T, typename E> using Expected = std::__1::expected<T, E>;
template <typename E> using Unexpected           = std::__1::unexpected<E>;

#define TRY(expr)                                                      \
    ({                                                                 \
        auto&& _e = (expr);                                            \
        if (!_e.has_value()) return Unexpected{std::move(_e).error()}; \
        std::move(_e).value();                                         \
    })

} // namespace conch
