#include <algorithm>
#include <initializer_list>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "syntax/builtins.hh"
#include "syntax/lexer.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include <types.hh>

namespace ghoti::tests {

using syntax::TokenType;
using ExpectedLexeme = std::pair<TokenType, std::string_view>;

namespace {

auto test_lexer(std::string_view input, std::initializer_list<ExpectedLexeme> expecteds) -> void {
    syntax::Lexer l{input};
    for (const auto& [expected_tok, expected_slice] : expecteds) {
        const auto token = l.advance();
        CHECK(expected_tok == token.type);
        CHECK(expected_slice == token.slice);
    }

    // Should be true regardless of caller putting END in their list
    const auto& end_tok = l.advance();
    CHECK(end_tok.type == TokenType::END);
    CHECK(end_tok.slice == "");
}

} // namespace

TEST_CASE("Lexing illegal characters") {
    syntax::Lexer l{"月😭🎶"};
    const auto    tokens = l.consume();

    for (usize i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (i == tokens.size() - 1) {
            CHECK(token.type == TokenType::END);
            break;
        }
        CHECK(token.type == TokenType::ILLEGAL);
    }
}

TEST_CASE("Lexer over-consumption") {
    syntax::Lexer l{"Lexer"};
    l.consume();
    for (usize i = 0; i < 100; ++i) { CHECK(l.advance().type == TokenType::END); }
}

TEST_CASE("Lexing symbols") {
    test_lexer("=+(){}[],;: !-/*<>_",
               {
                   {TokenType::ASSIGN, "="},
                   {TokenType::PLUS, "+"},
                   {TokenType::LPAREN, "("},
                   {TokenType::RPAREN, ")"},
                   {TokenType::LBRACE, "{"},
                   {TokenType::RBRACE, "}"},
                   {TokenType::LBRACKET, "["},
                   {TokenType::RBRACKET, "]"},
                   {TokenType::COMMA, ","},
                   {TokenType::SEMICOLON, ";"},
                   {TokenType::COLON, ":"},
                   {TokenType::BANG, "!"},
                   {TokenType::MINUS, "-"},
                   {TokenType::SLASH, "/"},
                   {TokenType::STAR, "*"},
                   {TokenType::LT, "<"},
                   {TokenType::GT, ">"},
                   {TokenType::UNDERSCORE, "_"},
               });
}

TEST_CASE("Lexing basic language snippet") {
    test_lexer("const five := 5;\n"
               "var ten := 10;\n\n"
               "var add := fn(x: i32, y: i32): i32 {\n"
               "   return x + y;\n"
               "};\n\n"
               "var result := add(five, ten);\n"
               "var fs: f64 = 4.2;",
               {
                   {TokenType::CONSTANT, "const"}, {TokenType::IDENT, "five"},
                   {TokenType::WALRUS, ":="},      {TokenType::INT_10, "5"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::VAR, "var"},
                   {TokenType::IDENT, "ten"},      {TokenType::WALRUS, ":="},
                   {TokenType::INT_10, "10"},      {TokenType::SEMICOLON, ";"},
                   {TokenType::VAR, "var"},        {TokenType::IDENT, "add"},
                   {TokenType::WALRUS, ":="},      {TokenType::FUNCTION, "fn"},
                   {TokenType::LPAREN, "("},       {TokenType::IDENT, "x"},
                   {TokenType::COLON, ":"},        {TokenType::I32_TYPE, "i32"},
                   {TokenType::COMMA, ","},        {TokenType::IDENT, "y"},
                   {TokenType::COLON, ":"},        {TokenType::I32_TYPE, "i32"},
                   {TokenType::RPAREN, ")"},       {TokenType::COLON, ":"},
                   {TokenType::I32_TYPE, "i32"},   {TokenType::LBRACE, "{"},
                   {TokenType::RETURN, "return"},  {TokenType::IDENT, "x"},
                   {TokenType::PLUS, "+"},         {TokenType::IDENT, "y"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::RBRACE, "}"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::VAR, "var"},
                   {TokenType::IDENT, "result"},   {TokenType::WALRUS, ":="},
                   {TokenType::IDENT, "add"},      {TokenType::LPAREN, "("},
                   {TokenType::IDENT, "five"},     {TokenType::COMMA, ","},
                   {TokenType::IDENT, "ten"},      {TokenType::RPAREN, ")"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::VAR, "var"},
                   {TokenType::IDENT, "fs"},       {TokenType::COLON, ":"},
                   {TokenType::F64_TYPE, "f64"},   {TokenType::ASSIGN, "="},
                   {TokenType::F64, "4.2"},        {TokenType::SEMICOLON, ";"},
               });
}

TEST_CASE("Lexing numbers") {
    test_lexer("0 123 3.14 42.0 1e20 1.e-3 2.3901E4f 1e.",
               {
                   {TokenType::INT_10, "0"},
                   {TokenType::INT_10, "123"},
                   {TokenType::F64, "3.14"},
                   {TokenType::F64, "42.0"},
                   {TokenType::F64, "1e20"},
                   {TokenType::F64, "1.e-3"},
                   {TokenType::F32, "2.3901E4f"},
                   {TokenType::INT_10, "1"},
                   {TokenType::IDENT, "e"},
                   {TokenType::DOT, "."},
               });
}

TEST_CASE("Lexing illegal floats") {
    test_lexer(".0 1..2 3.4.5 3.4u 5f",
               {
                   {TokenType::DOT, "."},
                   {TokenType::INT_10, "0"},
                   {TokenType::INT_10, "1"},
                   {TokenType::DOT_DOT, ".."},
                   {TokenType::INT_10, "2"},
                   {TokenType::F64, "3.4"},
                   {TokenType::DOT, "."},
                   {TokenType::INT_10, "5"},
                   {TokenType::F64, "3.4"},
                   {TokenType::IDENT, "u"},
                   {TokenType::F32, "5f"},
               });
}

TEST_CASE("Lexing signed int variants") {
    test_lexer("0b1010 0o17 0O17 42 0x2A 0X2A 0b 0x 0o",
               {
                   {TokenType::INT_2, "0b1010"},
                   {TokenType::INT_8, "0o17"},
                   {TokenType::INT_8, "0O17"},
                   {TokenType::INT_10, "42"},
                   {TokenType::INT_16, "0x2A"},
                   {TokenType::INT_16, "0X2A"},
                   {TokenType::ILLEGAL, "0b"},
                   {TokenType::ILLEGAL, "0x"},
                   {TokenType::ILLEGAL, "0o"},
               });
}

TEST_CASE("Lexing unsigned int variants") {
    test_lexer("0b1010u 0b1010uz 0o17u 0o17uz 0O17u 42u 42UZ 0x2AU 0X2Au 123ufoo 0bu 0xu 0ou",
               {
                   {TokenType::UINT_2, "0b1010u"},
                   {TokenType::UZINT_2, "0b1010uz"},
                   {TokenType::UINT_8, "0o17u"},
                   {TokenType::UZINT_8, "0o17uz"},
                   {TokenType::UINT_8, "0O17u"},
                   {TokenType::UINT_10, "42u"},
                   {TokenType::UZINT_10, "42UZ"},
                   {TokenType::UINT_16, "0x2AU"},
                   {TokenType::UINT_16, "0X2Au"},
                   {TokenType::UINT_10, "123u"},
                   {TokenType::IDENT, "foo"},
                   {TokenType::ILLEGAL, "0b"},
                   {TokenType::IDENT, "u"},
                   {TokenType::ILLEGAL, "0x"},
                   {TokenType::IDENT, "u"},
                   {TokenType::ILLEGAL, "0o"},
                   {TokenType::IDENT, "u"},
               });
}

TEST_CASE("Lexing int bit-width variants") {
    test_lexer("2 2l 2z 2u 2ul 2uz",
               {
                   {TokenType::INT_10, "2"},
                   {TokenType::LINT_10, "2l"},
                   {TokenType::ZINT_10, "2z"},
                   {TokenType::UINT_10, "2u"},
                   {TokenType::ULINT_10, "2ul"},
                   {TokenType::UZINT_10, "2uz"},
               });
}

TEST_CASE("Lexing underscore-separated numbers") {
    test_lexer("0_1 123_2 3.14_159 42.0_11f 0b11_00_11 0xEEEE_FFFFuz 0o7_233u",
               {
                   {TokenType::INT_10, "0_1"},
                   {TokenType::INT_10, "123_2"},
                   {TokenType::F64, "3.14_159"},
                   {TokenType::F32, "42.0_11f"},
                   {TokenType::INT_2, "0b11_00_11"},
                   {TokenType::UZINT_16, "0xEEEE_FFFFuz"},
                   {TokenType::UINT_8, "0o7_233u"},
               });
}

TEST_CASE("Lexing illegal underscored numbers") {
    test_lexer("1234_.1 121.3_ _12",
               {
                   {TokenType::ILLEGAL, "1234_"},
                   {TokenType::DOT, "."},
                   {TokenType::INT_10, "1"},
                   {TokenType::ILLEGAL, "121.3_"},
                   {TokenType::UNDERSCORE, "_"},
                   {TokenType::INT_10, "12"},
               });
}

TEST_CASE("Lexing keywords") {
    test_lexer("and or pub extern export volatile mut_volatile "
               "i32 i64 isize u32 u64 usize f32 f64 u8 bool void type test",
               {
                   {TokenType::BOOLEAN_AND, "and"},
                   {TokenType::BOOLEAN_OR, "or"},
                   {TokenType::PUBLIC, "pub"},
                   {TokenType::EXTERN, "extern"},
                   {TokenType::EXPORT, "export"},
                   {TokenType::VOLATILE, "volatile"},
                   {TokenType::MUT_VOLATILE, "mut_volatile"},
                   {TokenType::I32_TYPE, "i32"},
                   {TokenType::I64_TYPE, "i64"},
                   {TokenType::ISIZE_TYPE, "isize"},
                   {TokenType::U32_TYPE, "u32"},
                   {TokenType::U64_TYPE, "u64"},
                   {TokenType::USIZE_TYPE, "usize"},
                   {TokenType::F32_TYPE, "f32"},
                   {TokenType::F64_TYPE, "f64"},
                   {TokenType::U8_TYPE, "u8"},
                   {TokenType::BOOL_TYPE, "bool"},
                   {TokenType::VOID_TYPE, "void"},
                   {TokenType::TYPE_TYPE, "type"},
                   {TokenType::TEST, "test"},
               });
}

TEST_CASE("Lexing comments") {
    test_lexer("const five := 5;\n"
               "var ten_10 := 10;\n\n"
               "// BOL\n"
               "var result := add(five, ten); // EOL\n"
               "var four_and_some := 4.2f;",
               {
                   {TokenType::CONSTANT, "const"}, {TokenType::IDENT, "five"},
                   {TokenType::WALRUS, ":="},      {TokenType::INT_10, "5"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::VAR, "var"},
                   {TokenType::IDENT, "ten_10"},   {TokenType::WALRUS, ":="},
                   {TokenType::INT_10, "10"},      {TokenType::SEMICOLON, ";"},
                   {TokenType::COMMENT, " BOL"},   {TokenType::VAR, "var"},
                   {TokenType::IDENT, "result"},   {TokenType::WALRUS, ":="},
                   {TokenType::IDENT, "add"},      {TokenType::LPAREN, "("},
                   {TokenType::IDENT, "five"},     {TokenType::COMMA, ","},
                   {TokenType::IDENT, "ten"},      {TokenType::RPAREN, ")"},
                   {TokenType::SEMICOLON, ";"},    {TokenType::COMMENT, " EOL"},
                   {TokenType::VAR, "var"},        {TokenType::IDENT, "four_and_some"},
                   {TokenType::WALRUS, ":="},      {TokenType::F32, "4.2f"},
                   {TokenType::SEMICOLON, ";"},
               });
}

TEST_CASE("Lexing character literals") {
    test_lexer("if'e' else'\\'\nreturn'\\r' break'\\n'\n"
               "continue'\\0' for'\\'' while'\\\\' const''\n"
               "var'asd'",
               {
                   {TokenType::IF, "if"},
                   {TokenType::U8, "'e'"},
                   {TokenType::ELSE, "else"},
                   {TokenType::ILLEGAL, "'\\'"},
                   {TokenType::RETURN, "return"},
                   {TokenType::U8, "'\\r'"},
                   {TokenType::BREAK, "break"},
                   {TokenType::U8, "'\\n'"},
                   {TokenType::CONTINUE, "continue"},
                   {TokenType::U8, "'\\0'"},
                   {TokenType::FOR, "for"},
                   {TokenType::U8, "'\\''"},
                   {TokenType::WHILE, "while"},
                   {TokenType::U8, "'\\\\'"},
                   {TokenType::CONSTANT, "const"},
                   {TokenType::ILLEGAL, "'"},
                   {TokenType::ILLEGAL, "'"},
                   {TokenType::VAR, "var"},
                   {TokenType::ILLEGAL, "'asd'"},
               });
}

TEST_CASE("Lexing string literals") {
    test_lexer(
        R"("This is a string";const five := "Hello, World!";var ten: [:0]u8 = "Hello\n, World!\0";var one := "Hello, World!;)",
        {
            {TokenType::STRING, R"("This is a string")"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::CONSTANT, "const"},
            {TokenType::IDENT, "five"},
            {TokenType::WALRUS, ":="},
            {TokenType::STRING, R"("Hello, World!")"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},
            {TokenType::COLON, ":"},
            {TokenType::LBRACKET, "["},
            {TokenType::NULL_TERMINATED, ":0"},
            {TokenType::RBRACKET, "]"},
            {TokenType::U8_TYPE, "u8"},
            {TokenType::ASSIGN, "="},
            {TokenType::STRING, R"("Hello\n, World!\0")"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "one"},
            {TokenType::WALRUS, ":="},
            {TokenType::ILLEGAL, R"("Hello, World!;)"},
        });
}

TEST_CASE("Lexing multiline string literals") {
    test_lexer("const five := \\\\Multiline stringing\n"
               ";\n"
               "var ten := \\\\Multiline stringing\n"
               "\\\\Continuation\n"
               ";\n"
               "const one := \\\\Nesting \" \' \\ [] const var\n"
               "\\\\\n"
               ";\n",
               {
                   {TokenType::CONSTANT, "const"},
                   {TokenType::IDENT, "five"},
                   {TokenType::WALRUS, ":="},
                   {TokenType::MULTILINE_STRING, "Multiline stringing"},
                   {TokenType::SEMICOLON, ";"},
                   {TokenType::VAR, "var"},
                   {TokenType::IDENT, "ten"},
                   {TokenType::WALRUS, ":="},
                   {TokenType::MULTILINE_STRING, "Multiline stringing\n\\\\Continuation"},
                   {TokenType::SEMICOLON, ";"},
                   {TokenType::CONSTANT, "const"},
                   {TokenType::IDENT, "one"},
                   {TokenType::WALRUS, ":="},
                   {TokenType::MULTILINE_STRING, "Nesting \" \' \\ [] const var\n\\\\"},
                   {TokenType::SEMICOLON, ";"},
               });
}

TEST_CASE("Lexing pointers and references") {
    test_lexer("& &mut * ^ ^mut nullptr",
               {
                   {TokenType::BW_AND, "&"},
                   {TokenType::AND_MUT, "&mut"},
                   {TokenType::STAR, "*"},
                   {TokenType::CARET, "^"},
                   {TokenType::CARET_MUT, "^mut"},
                   {TokenType::NULLPTR, "nullptr"},
               });
}

TEST_CASE("Lexing compiler builtins & Lexer resetting") {
    constexpr auto expecteds = std::views::transform(
        syntax::builtins::ALL_TOKEN_TYPES,
        [](const auto& tt) -> syntax::Builtin { return {*syntax::get_builtin_opt(tt), tt}; });

    std::string input;
    std::ranges::for_each(expecteds, [&input](const auto& lexeme) -> void {
        input.append(lexeme.name);
        input.push_back(' ');
    });
    syntax::Lexer l{input};

    syntax::Lexer l_accumulator{input};
    const auto    accumulated_tokens = l_accumulator.consume();
    l_accumulator.reset(input);
    const auto reset_acc = l_accumulator.consume();

    for (usize i = 0; const auto& [expected_slice, expected_tt] : expecteds) {
        const auto  token             = l.advance();
        const auto& accumulated_token = accumulated_tokens[i];
        const auto& reset             = reset_acc[i];

        CHECK(expected_tt == token.type);
        CHECK(expected_tt == accumulated_token.type);
        CHECK(expected_slice == token.slice);
        CHECK(expected_slice == accumulated_token.slice);
        CHECK(accumulated_token == reset);
        i += 1;
    }
}

TEST_CASE("Lexing illegal builtin") {
    const std::string_view input{"@run"};
    syntax::Lexer          l{input};
    const auto             illegal = l.advance();
    CHECK(TokenType::ILLEGAL == illegal.type);
    CHECK(input == illegal.slice);
}

} // namespace ghoti::tests
