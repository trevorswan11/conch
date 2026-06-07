#include "syntax/builtins.hh"

#include <string_view>

#include "syntax/token_type.hh"

#include "fixed/enum_map.hh"
#include "fixed/hash_map.hh"
#include "option.hh"

namespace ghoti::syntax {

namespace {

constexpr auto ALL_BUILTINS_BY_SV = fixed::make_hash_map(builtins::ALIGN_CAST,
                                                         builtins::PTR_CAST,
                                                         builtins::BIT_CAST,
                                                         builtins::CONST_CAST,
                                                         builtins::VOLATILE_CAST,
                                                         builtins::AS,
                                                         builtins::INT_FROM_PTR,
                                                         builtins::PTR_FROM_INT,
                                                         builtins::PTR_FROM_ARRAY,
                                                         builtins::SLICE_FROM_PTR,
                                                         builtins::ALIGN_OF,
                                                         builtins::SIZE_OF,
                                                         builtins::TYPE_OF,
                                                         builtins::THIS,
                                                         builtins::TAG_NAME,
                                                         builtins::MEMCPY,
                                                         builtins::MEMSET,
                                                         builtins::MEMMOVE,
                                                         builtins::MUL_ADD,
                                                         builtins::CLZ,
                                                         builtins::CTZ,
                                                         builtins::POP_COUNT,
                                                         builtins::SQRT,
                                                         builtins::SIN,
                                                         builtins::COS,
                                                         builtins::TAN,
                                                         builtins::EXP,
                                                         builtins::EXP2,
                                                         builtins::LOG,
                                                         builtins::LOG2,
                                                         builtins::LOG10,
                                                         builtins::ABS,
                                                         builtins::FLOOR,
                                                         builtins::CEIL,
                                                         builtins::PANIC);

constexpr auto ALL_BUILTINS_BY_TT = [] {
    fixed::EnumMap<TokenType, opt::Option<std::string_view>> builtins;
    for (const auto& [name, tok] : ALL_BUILTINS_BY_SV) { builtins[tok] = name; }
    return builtins;
}();

} // namespace

auto get_builtin_opt(TokenType tt) noexcept -> opt::Option<std::string_view> {
    return ALL_BUILTINS_BY_TT[tt];
}

auto get_builtin_opt(std::string_view sv) noexcept -> opt::Option<TokenType> {
    return ALL_BUILTINS_BY_SV.get_opt(sv).materialize();
}

} // namespace ghoti::syntax
