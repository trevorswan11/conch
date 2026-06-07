#pragma once

#include "syntax/token_type.hh"

#include <option.hh>
#include <types.hh>

namespace ghoti::syntax {

enum class Precedence : u8 {
    LOWEST,
    ASSIGNMENT       = 10,
    BOOL_AND_OR      = 20,
    BOOL_EQUIV       = 30,
    BOOL_LT_GT       = 40,
    ADD_SUB          = 50,
    MUL_DIV          = 60,
    EXPONENT         = 70,
    PREFIX           = 80,
    RANGE            = 90,
    INITIALIZATION   = 100,
    TYPE             = 110,
    SCOPE_RESOLUTION = 120,
    GROUP_CALL_IDX   = 130,
    LABEL            = 140,
};

struct Binding {
    Precedence precedence;
    bool       right_assoc{false};

    [[nodiscard]] static auto try_get_from(TokenType tt) noexcept -> opt::Option<Binding>;
};

} // namespace ghoti::syntax
