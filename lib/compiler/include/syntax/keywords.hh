#pragma once

#include <array>
#include <string_view>

#include "syntax/token_type.hh"

#include "option.hh"

namespace porpoise::syntax {

using Keyword = TypedIdentifier;

namespace keywords {

constexpr Keyword FN{"fn", TokenType::FUNCTION};
constexpr Keyword VAR{"var", TokenType::VAR};
constexpr Keyword CONSTANT{"const", TokenType::CONSTANT};
constexpr Keyword CONSTEXPR{"constexpr", TokenType::CONSTEXPR};
constexpr Keyword STRUCT{"struct", TokenType::STRUCT};
constexpr Keyword ENUM{"enum", TokenType::ENUM};
constexpr Keyword UNION{"union", TokenType::UNION};
constexpr Keyword BOOLEAN_TRUE{"true", TokenType::BOOLEAN_TRUE};
constexpr Keyword BOOLEAN_FALSE{"false", TokenType::BOOLEAN_FALSE};
constexpr Keyword IF{"if", TokenType::IF};
constexpr Keyword ELSE{"else", TokenType::ELSE};
constexpr Keyword DO{"do", TokenType::DO};
constexpr Keyword MATCH{"match", TokenType::MATCH};
constexpr Keyword RETURN{"return", TokenType::RETURN};
constexpr Keyword DEFER{"defer", TokenType::DEFER};
constexpr Keyword LOOP{"loop", TokenType::LOOP};
constexpr Keyword FOR{"for", TokenType::FOR};
constexpr Keyword WHILE{"while", TokenType::WHILE};
constexpr Keyword CONTINUE{"continue", TokenType::CONTINUE};
constexpr Keyword BREAK{"break", TokenType::BREAK};
constexpr Keyword IMPORT{"import", TokenType::IMPORT};
constexpr Keyword I32{"i32", TokenType::I32_TYPE};
constexpr Keyword I64{"i64", TokenType::I64_TYPE};
constexpr Keyword ISIZE{"isize", TokenType::ISIZE_TYPE};
constexpr Keyword U32{"u32", TokenType::U32_TYPE};
constexpr Keyword U64{"u64", TokenType::U64_TYPE};
constexpr Keyword USIZE{"usize", TokenType::USIZE_TYPE};
constexpr Keyword F32{"f32", TokenType::F32_TYPE};
constexpr Keyword F64{"f64", TokenType::F64_TYPE};
constexpr Keyword U8{"u8", TokenType::U8_TYPE};
constexpr Keyword BOOL{"bool", TokenType::BOOL_TYPE};
constexpr Keyword VOID{"void", TokenType::VOID_TYPE};
constexpr Keyword TYPE{"type", TokenType::TYPE_TYPE};
constexpr Keyword AUTO{"auto", TokenType::AUTO_TYPE};
constexpr Keyword OPAQUE{"opaque", TokenType::OPAQUE_TYPE};
constexpr Keyword AS{"as", TokenType::AS};
constexpr Keyword PUBLIC{"pub", TokenType::PUBLIC};
constexpr Keyword EXTERN{"extern", TokenType::EXTERN};
constexpr Keyword EXPORT{"export", TokenType::EXPORT};
constexpr Keyword VOLATILE{"volatile", TokenType::VOLATILE};
constexpr Keyword MUT_VOLATILE{"mut_volatile", TokenType::MUT_VOLATILE};
constexpr Keyword NORETURN{"noreturn", TokenType::NORETURN};
constexpr Keyword NULLPTR{"nullptr", TokenType::NULLPTR};
constexpr Keyword USING{"using", TokenType::USING};
constexpr Keyword TEST{"test", TokenType::TEST};
constexpr Keyword UNDEFINED{"undefined", TokenType::UNDEFINED};

} // namespace keywords

[[nodiscard]] auto get_keyword_opt(std::string_view sv) noexcept -> opt::Option<TokenType>;

constexpr auto ALL_PRIMITIVES = std::array{
    keywords::I32.type,
    keywords::I64.type,
    keywords::ISIZE.type,
    keywords::U32.type,
    keywords::U64.type,
    keywords::USIZE.type,
    keywords::F32.type,
    keywords::F64.type,
    keywords::U8.type,
    keywords::BOOL.type,
    keywords::VOID.type,
};

} // namespace porpoise::syntax
