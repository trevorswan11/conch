#pragma once

#include <string_view>

#include <stdx/enum.hh>
#include <stdx/option.hh>

#include "syntax/token_type.hh"

namespace ghoti::syntax {

using Builtin = TypedIdentifier;

namespace builtins {

constexpr Builtin ALIGN_CAST{"@alignCast", TokenType::BUILTIN_ALIGN_CAST};
constexpr Builtin PTR_CAST{"@ptrCast", TokenType::BUILTIN_PTR_CAST};
constexpr Builtin BIT_CAST{"@bitCast", TokenType::BUILTIN_BIT_CAST};
constexpr Builtin CONST_CAST{"@constCast", TokenType::BUILTIN_CONST_CAST};
constexpr Builtin VOLATILE_CAST{"@volatileCast", TokenType::BUILTIN_VOLATILE_CAST};
constexpr Builtin AS{"@as", TokenType::BUILTIN_AS};

constexpr Builtin INT_FROM_PTR{"@intFromPtr", TokenType::BUILTIN_INT_FROM_PTR};
constexpr Builtin PTR_FROM_INT{"@ptrFromInt", TokenType::BUILTIN_PTR_FROM_INT};
constexpr Builtin PTR_FROM_ARRAY{"@ptrFromArray", TokenType::BUILTIN_PTR_FROM_ARRAY};
constexpr Builtin SLICE_FROM_PTR{"@sliceFromPtr", TokenType::BUILTIN_SLICE_FROM_PTR};

constexpr Builtin ALIGN_OF{"@alignOf", TokenType::BUILTIN_ALIGN_OF};
constexpr Builtin SIZE_OF{"@sizeOf", TokenType::BUILTIN_SIZE_OF};
constexpr Builtin TYPE_OF{"@typeOf", TokenType::BUILTIN_TYPE_OF};
constexpr Builtin THIS{"@this", TokenType::BUILTIN_THIS};
constexpr Builtin TAG_NAME{"@tagName", TokenType::BUILTIN_TAG_NAME};

constexpr Builtin MEMCPY{"@memcpy", TokenType::BUILTIN_MEMCPY};
constexpr Builtin MEMSET{"@memset", TokenType::BUILTIN_MEMSET};
constexpr Builtin MEMMOVE{"@memmove", TokenType::BUILTIN_MEMMOVE};

constexpr Builtin MUL_ADD{"@mulAdd", TokenType::BUILTIN_MUL_ADD};
constexpr Builtin CLZ{"@clz", TokenType::BUILTIN_CLZ};
constexpr Builtin CTZ{"@ctz", TokenType::BUILTIN_CTZ};
constexpr Builtin POP_COUNT{"@popCount", TokenType::BUILTIN_POP_COUNT};
constexpr Builtin SQRT{"@sqrt", TokenType::BUILTIN_SQRT};
constexpr Builtin SIN{"@sin", TokenType::BUILTIN_SIN};
constexpr Builtin COS{"@cos", TokenType::BUILTIN_COS};
constexpr Builtin TAN{"@tan", TokenType::BUILTIN_TAN};
constexpr Builtin EXP{"@exp", TokenType::BUILTIN_EXP};
constexpr Builtin EXP2{"@exp2", TokenType::BUILTIN_EXP2};
constexpr Builtin LOG{"@log", TokenType::BUILTIN_LOG};
constexpr Builtin LOG2{"@log2", TokenType::BUILTIN_LOG2};
constexpr Builtin LOG10{"@log10", TokenType::BUILTIN_LOG10};
constexpr Builtin ABS{"@abs", TokenType::BUILTIN_ABS};
constexpr Builtin FLOOR{"@floor", TokenType::BUILTIN_FLOOR};
constexpr Builtin CEIL{"@ceil", TokenType::BUILTIN_CEIL};

constexpr Builtin PANIC{"@panic", TokenType::BUILTIN_PANIC};

constexpr auto ALL_TOKEN_TYPES{
    stdx::enum_range<TokenType::BUILTIN_ALIGN_CAST, TokenType::BUILTIN_PANIC>()};

} // namespace builtins

[[nodiscard]] auto get_builtin_opt(TokenType tt) noexcept -> stdx::option<std::string_view>;
[[nodiscard]] auto get_builtin_opt(std::string_view sv) noexcept -> stdx::option<TokenType>;

} // namespace ghoti::syntax
