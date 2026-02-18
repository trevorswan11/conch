#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "optional.hpp"

#include "lexer/token.hpp"

namespace conch {

using Keyword = std::pair<std::string_view, TokenType>;

namespace keywords {

constexpr Keyword FN{"fn", TokenType::FUNCTION};
constexpr Keyword VAR{"var", TokenType::VAR};
constexpr Keyword CONST{"const", TokenType::CONST};
constexpr Keyword STRUCT{"struct", TokenType::STRUCT};
constexpr Keyword ENUM{"enum", TokenType::ENUM};
constexpr Keyword TRUE{"true", TokenType::TRUE};
constexpr Keyword FALSE{"false", TokenType::FALSE};
constexpr Keyword BOOLEAN_AND{"and", TokenType::BOOLEAN_AND};
constexpr Keyword BOOLEAN_OR{"or", TokenType::BOOLEAN_OR};
constexpr Keyword IS{"is", TokenType::IS};
constexpr Keyword IN{"in", TokenType::IN};
constexpr Keyword IF{"if", TokenType::IF};
constexpr Keyword ELSE{"else", TokenType::ELSE};
constexpr Keyword ORELSE{"orelse", TokenType::ORELSE};
constexpr Keyword DO{"do", TokenType::DO};
constexpr Keyword MATCH{"match", TokenType::MATCH};
constexpr Keyword RETURN{"return", TokenType::RETURN};
constexpr Keyword LOOP{"loop", TokenType::LOOP};
constexpr Keyword FOR{"for", TokenType::FOR};
constexpr Keyword WHILE{"while", TokenType::WHILE};
constexpr Keyword CONTINUE{"continue", TokenType::CONTINUE};
constexpr Keyword BREAK{"break", TokenType::BREAK};
constexpr Keyword IMPORT{"import", TokenType::IMPORT};
constexpr Keyword INT{"int", TokenType::INT_TYPE};
constexpr Keyword LONG{"long", TokenType::LONG_TYPE};
constexpr Keyword ISIZE{"isize", TokenType::ISIZE_TYPE};
constexpr Keyword UINT{"uint", TokenType::UINT_TYPE};
constexpr Keyword ULONG{"ulong", TokenType::ULONG_TYPE};
constexpr Keyword USIZE{"usize", TokenType::USIZE_TYPE};
constexpr Keyword FLOAT{"float", TokenType::FLOAT_TYPE};
constexpr Keyword BYTE{"byte", TokenType::BYTE_TYPE};
constexpr Keyword STRING{"string", TokenType::STRING_TYPE};
constexpr Keyword BOOL{"bool", TokenType::BOOL_TYPE};
constexpr Keyword VOID{"void", TokenType::VOID_TYPE};
constexpr Keyword TYPE{"type", TokenType::TYPE_TYPE};
constexpr Keyword AS{"as", TokenType::AS};
constexpr Keyword PRIVATE{"private", TokenType::PRIVATE};
constexpr Keyword EXTERN{"extern", TokenType::EXTERN};
constexpr Keyword EXPORT{"export", TokenType::EXPORT};
constexpr Keyword PACKED{"packed", TokenType::PACKED};
constexpr Keyword VOLATILE{"volatile", TokenType::VOLATILE};
constexpr Keyword STATIC{"static", TokenType::STATIC};
constexpr Keyword MUT{"mut", TokenType::MUT};
constexpr Keyword REF{"ref", TokenType::REF};
constexpr Keyword NORETURN{"noreturn", TokenType::NORETURN};

namespace builtins {

constexpr Keyword TYPEOF{"@typeOf", TokenType::TYPEOF};
constexpr Keyword SIZEOF{"@sizeOf", TokenType::SIZEOF};
constexpr Keyword ALIGNOF{"@alignOf", TokenType::ALIGNOF};
constexpr Keyword SIN{"@sin", TokenType::SIN};
constexpr Keyword COS{"@cos", TokenType::COS};
constexpr Keyword TAN{"@tan", TokenType::TAN};
constexpr Keyword SQRT{"@sqrt", TokenType::SQRT};
constexpr Keyword LOG{"@log", TokenType::LOG};
constexpr Keyword LOG_10{"@log10", TokenType::LOG_10};
constexpr Keyword LOG_2{"@log2", TokenType::LOG_2};
constexpr Keyword MIN{"@min", TokenType::MIN};
constexpr Keyword MAX{"@max", TokenType::MAX};
constexpr Keyword MOD{"@mod", TokenType::MOD};
constexpr Keyword DIVMOD{"@divmod", TokenType::DIVMOD};
constexpr Keyword TRUNC{"@trunc", TokenType::TRUNC};
constexpr Keyword CAST{"@cast", TokenType::CAST};
constexpr Keyword CEIL{"@ceil", TokenType::CEIL};
constexpr Keyword FLOOR{"@floor", TokenType::FLOOR};
constexpr Keyword EXP{"@exp", TokenType::EXP};
constexpr Keyword EXP_2{"@exp2", TokenType::EXP_2};
constexpr Keyword CLZ{"@clz", TokenType::CLZ};
constexpr Keyword CTZ{"@ctz", TokenType::CTZ};

} // namespace builtins

} // namespace keywords

constexpr auto ALL_KEYWORDS = []() {
    auto all_keywords = std::array{
        keywords::FN,         keywords::VAR,    keywords::CONST,  keywords::STRUCT,
        keywords::ENUM,       keywords::TRUE,   keywords::FALSE,  keywords::BOOLEAN_AND,
        keywords::BOOLEAN_OR, keywords::IS,     keywords::IN,     keywords::IF,
        keywords::ELSE,       keywords::ORELSE, keywords::DO,     keywords::MATCH,
        keywords::RETURN,     keywords::LOOP,   keywords::FOR,    keywords::WHILE,
        keywords::CONTINUE,   keywords::BREAK,  keywords::IMPORT, keywords::INT,
        keywords::LONG,       keywords::ISIZE,  keywords::UINT,   keywords::ULONG,
        keywords::USIZE,      keywords::FLOAT,  keywords::BYTE,   keywords::STRING,
        keywords::BOOL,       keywords::VOID,   keywords::TYPE,   keywords::AS,
        keywords::PRIVATE,    keywords::EXTERN, keywords::EXPORT, keywords::PACKED,
        keywords::VOLATILE,   keywords::STATIC, keywords::MUT,    keywords::REF,
        keywords::NORETURN,
    };

    std::ranges::sort(all_keywords, {}, &Keyword::first);
    return all_keywords;
}();

constexpr auto get_keyword(std::string_view sv) noexcept -> Optional<Keyword> {
    const auto it = std::ranges::lower_bound(ALL_KEYWORDS, sv, {}, &Keyword::first);
    if (it == ALL_KEYWORDS.end() || it->first != sv) { return nullopt; }
    return Optional<Keyword>{*it};
}

constexpr auto ALL_PRIMITIVES = std::array{
    keywords::INT.second,
    keywords::LONG.second,
    keywords::ISIZE.second,
    keywords::UINT.second,
    keywords::ULONG.second,
    keywords::USIZE.second,
    keywords::FLOAT.second,
    keywords::BYTE.second,
    keywords::STRING.second,
    keywords::BOOL.second,
    keywords::VOID.second,
};

constexpr auto ALL_BUILTINS = []() {
    using namespace keywords;
    auto all_builtins = std::array{
        builtins::TYPEOF, builtins::SIZEOF, builtins::ALIGNOF, builtins::SIN,    builtins::COS,
        builtins::TAN,    builtins::SQRT,   builtins::LOG,     builtins::LOG_10, builtins::LOG_2,
        builtins::MIN,    builtins::MAX,    builtins::MOD,     builtins::DIVMOD, builtins::TRUNC,
        builtins::CAST,   builtins::CEIL,   builtins::FLOOR,   builtins::EXP,    builtins::EXP_2,
        builtins::CLZ,    builtins::CTZ,
    };

    std::ranges::sort(all_builtins, {}, &Keyword::first);
    return all_builtins;
}();

constexpr auto get_builtin(std::string_view sv) noexcept -> Optional<Keyword> {
    const auto it = std::ranges::lower_bound(ALL_BUILTINS, sv, {}, &Keyword::first);
    if (it == ALL_BUILTINS.end() || it->first != sv) { return nullopt; }
    return Optional<Keyword>{*it};
}

constexpr auto is_builtin(TokenType tt) noexcept -> bool {
    constexpr auto ALL_BUILTINS_BY_TT = [] {
        auto arr = ALL_BUILTINS;
        std::ranges::sort(arr, {}, &Keyword::second);
        return arr;
    }();

    const auto it = std::ranges::lower_bound(ALL_BUILTINS_BY_TT, tt, {}, &Keyword::second);
    return it != ALL_BUILTINS_BY_TT.end() && it->second == tt;
}

} // namespace conch
