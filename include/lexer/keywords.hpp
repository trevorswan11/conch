#pragma once

#include <array>
#include <string_view>
#include <utility>

#include "core.hpp"

#include "lexer/token.hpp"

using Keyword    = std::pair<std::string_view, TokenType>;
using KeywordMap = flat_map_from_pair<Keyword>;

namespace keywords {
inline constexpr Keyword FN{"fn", TokenType::FUNCTION};
inline constexpr Keyword VAR{"var", TokenType::VAR};
inline constexpr Keyword CONST{"const", TokenType::CONST};
inline constexpr Keyword STRUCT{"struct", TokenType::STRUCT};
inline constexpr Keyword ENUM{"enum", TokenType::ENUM};
inline constexpr Keyword TRUE{"true", TokenType::TRUE};
inline constexpr Keyword FALSE{"false", TokenType::FALSE};
inline constexpr Keyword BOOLEAN_AND{"and", TokenType::BOOLEAN_AND};
inline constexpr Keyword BOOLEAN_OR{"or", TokenType::BOOLEAN_OR};
inline constexpr Keyword IS{"is", TokenType::IS};
inline constexpr Keyword IN{"in", TokenType::IN};
inline constexpr Keyword IF{"if", TokenType::IF};
inline constexpr Keyword ELSE{"else", TokenType::ELSE};
inline constexpr Keyword ORELSE{"orelse", TokenType::ORELSE};
inline constexpr Keyword DO{"do", TokenType::DO};
inline constexpr Keyword MATCH{"match", TokenType::MATCH};
inline constexpr Keyword RETURN{"return", TokenType::RETURN};
inline constexpr Keyword LOOP{"loop", TokenType::LOOP};
inline constexpr Keyword FOR{"for", TokenType::FOR};
inline constexpr Keyword WHILE{"while", TokenType::WHILE};
inline constexpr Keyword CONTINUE{"continue", TokenType::CONTINUE};
inline constexpr Keyword BREAK{"break", TokenType::BREAK};
inline constexpr Keyword NIL{"nil", TokenType::NIL};
inline constexpr Keyword TYPEOF{"typeof", TokenType::TYPEOF};
inline constexpr Keyword IMPL{"impl", TokenType::IMPL};
inline constexpr Keyword IMPORT{"import", TokenType::IMPORT};
inline constexpr Keyword INT{"int", TokenType::INT_TYPE};
inline constexpr Keyword UINT{"uint", TokenType::UINT_TYPE};
inline constexpr Keyword SIZE{"size", TokenType::SIZE_TYPE};
inline constexpr Keyword FLOAT{"float", TokenType::FLOAT_TYPE};
inline constexpr Keyword BYTE{"byte", TokenType::BYTE_TYPE};
inline constexpr Keyword STRING{"string", TokenType::STRING_TYPE};
inline constexpr Keyword BOOL{"bool", TokenType::BOOL_TYPE};
inline constexpr Keyword VOID{"void", TokenType::VOID_TYPE};
inline constexpr Keyword TYPE{"type", TokenType::TYPE};
inline constexpr Keyword WITH{"with", TokenType::WITH};
inline constexpr Keyword AS{"as", TokenType::AS};

inline constexpr auto RAW_KEYWORDS = std::array{
    keywords::FN,         keywords::VAR,    keywords::CONST, keywords::STRUCT,
    keywords::ENUM,       keywords::TRUE,   keywords::FALSE, keywords::BOOLEAN_AND,
    keywords::BOOLEAN_OR, keywords::IS,     keywords::IN,    keywords::IF,
    keywords::ELSE,       keywords::ORELSE, keywords::DO,    keywords::MATCH,
    keywords::RETURN,     keywords::LOOP,   keywords::FOR,   keywords::WHILE,
    keywords::CONTINUE,   keywords::BREAK,  keywords::NIL,   keywords::TYPEOF,
    keywords::IMPL,       keywords::IMPORT, keywords::INT,   keywords::UINT,
    keywords::SIZE,       keywords::FLOAT,  keywords::BYTE,  keywords::STRING,
    keywords::BOOL,       keywords::VOID,   keywords::TYPE,  keywords::WITH,
    keywords::AS,
};
} // namespace keywords

inline const KeywordMap ALL_KEYWORDS{keywords::RAW_KEYWORDS.begin(), keywords::RAW_KEYWORDS.end()};
