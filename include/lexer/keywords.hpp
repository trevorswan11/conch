#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>

#include "util/optional.hpp"

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
constexpr Keyword NIL{"nil", TokenType::NIL};
constexpr Keyword TYPEOF{"typeof", TokenType::TYPEOF};
constexpr Keyword IMPORT{"import", TokenType::IMPORT};
constexpr Keyword INT{"int", TokenType::INT_TYPE};
constexpr Keyword UINT{"uint", TokenType::UINT_TYPE};
constexpr Keyword SIZE{"size", TokenType::SIZE_TYPE};
constexpr Keyword FLOAT{"float", TokenType::FLOAT_TYPE};
constexpr Keyword BYTE{"byte", TokenType::BYTE_TYPE};
constexpr Keyword STRING{"string", TokenType::STRING_TYPE};
constexpr Keyword BOOL{"bool", TokenType::BOOL_TYPE};
constexpr Keyword VOID{"void", TokenType::VOID_TYPE};
constexpr Keyword TYPE{"type", TokenType::TYPE};
constexpr Keyword WITH{"with", TokenType::WITH};
constexpr Keyword AS{"as", TokenType::AS};
constexpr Keyword PRIVATE{"private", TokenType::PRIVATE};
constexpr Keyword EXTERN{"extern", TokenType::EXTERN};
constexpr Keyword EXPORT{"export", TokenType::EXPORT};
constexpr Keyword NAMESPACE{"namespace", TokenType::NAMESPACE};
constexpr Keyword PACKED{"packed", TokenType::PACKED};

} // namespace keywords

constexpr auto ALL_KEYWORDS = std::array{
    keywords::FN,         keywords::VAR,    keywords::CONST,  keywords::STRUCT,
    keywords::ENUM,       keywords::TRUE,   keywords::FALSE,  keywords::BOOLEAN_AND,
    keywords::BOOLEAN_OR, keywords::IS,     keywords::IN,     keywords::IF,
    keywords::ELSE,       keywords::ORELSE, keywords::DO,     keywords::MATCH,
    keywords::RETURN,     keywords::LOOP,   keywords::FOR,    keywords::WHILE,
    keywords::CONTINUE,   keywords::BREAK,  keywords::NIL,    keywords::TYPEOF,
    keywords::IMPORT,     keywords::INT,    keywords::UINT,   keywords::SIZE,
    keywords::FLOAT,      keywords::BYTE,   keywords::STRING, keywords::BOOL,
    keywords::VOID,       keywords::TYPE,   keywords::WITH,   keywords::AS,
    keywords::PRIVATE,    keywords::EXTERN, keywords::EXPORT, keywords::NAMESPACE,
    keywords::PACKED,
};

constexpr auto get_keyword(std::string_view sv) noexcept -> Optional<Keyword> {
    const auto it = std::ranges::find(ALL_KEYWORDS, sv, &Keyword::first);
    return it == ALL_KEYWORDS.end() ? nullopt : Optional<Keyword>{*it};
}

constexpr auto ALL_PRIMITIVES = std::array{
    keywords::INT.second,
    keywords::UINT.second,
    keywords::SIZE.second,
    keywords::FLOAT.second,
    keywords::BYTE.second,
    keywords::STRING.second,
    keywords::BOOL.second,
    keywords::VOID.second,
    keywords::NIL.second,
};

} // namespace conch
