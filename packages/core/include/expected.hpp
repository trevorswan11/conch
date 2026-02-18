#pragma once

#include <expected>

namespace conch {

template <typename T, typename E> using Expected = std::__1::expected<T, E>;
template <typename E> using Unexpected           = std::__1::unexpected<E>;

// A 'hack' to imitate the 'try' keyword in zig using GNU Statement Expressions
//  https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define TRY(expr)                                                      \
    ({                                                                 \
        auto&& _e = (expr);                                            \
        if (!_e.has_value()) return Unexpected{std::move(_e).error()}; \
        std::move(_e).value();                                         \
    })

} // namespace conch
