#include <ranges>
#include <string_view>
#include <utility>

#include <catch_amalgamated.hpp>

#include "lexer/keywords.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token.hpp"

namespace conch::tests {

using ExpectedLexeme = std::pair<TokenType, std::string_view>;

TEST_CASE("Illegal characters") {
    Lexer      l{"月😭🎶"};
    const auto tokens = l.consume();

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        if (i == tokens.size() - 1) {
            REQUIRE(token.type == TokenType::END);
            break;
        }
        REQUIRE(token.type == TokenType::ILLEGAL);
    }
}

TEST_CASE("Lexer over-consumption") {
    Lexer l{"lexer"};
    l.consume();
    for (size_t i = 0; i < 100; ++i) { REQUIRE(l.advance().type == TokenType::END); }
}

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Symbols Only") {
        Lexer l{"=+(){}[],;: !-/*<>_"};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::ASSIGN, "="},    {TokenType::PLUS, "+"},     {TokenType::LPAREN, "("},
            {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},   {TokenType::RBRACE, "}"},
            {TokenType::LBRACKET, "["},  {TokenType::RBRACKET, "]"}, {TokenType::COMMA, ","},
            {TokenType::SEMICOLON, ";"}, {TokenType::COLON, ":"},    {TokenType::BANG, "!"},
            {TokenType::MINUS, "-"},     {TokenType::SLASH, "/"},    {TokenType::STAR, "*"},
            {TokenType::LT, "<"},        {TokenType::GT, ">"},       {TokenType::UNDERSCORE, "_"},
            {TokenType::END, ""},
        });

        for (const auto& [expected_tok, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tok == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Naive language snippet") {
        const std::string_view input{"const five := 5;\n"
                                     "var ten := 10;\n\n"
                                     "var add := fn(x, y) {\n"
                                     "   x + y;\n"
                                     "};\n\n"
                                     "var result := add(five, ten);\n"
                                     "var four_and_some := 4.2;"};
        Lexer                  l{input};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::CONST, "const"}, {TokenType::IDENT, "five"},
            {TokenType::WALRUS, ":="},   {TokenType::INT_10, "5"},
            {TokenType::SEMICOLON, ";"}, {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},   {TokenType::WALRUS, ":="},
            {TokenType::INT_10, "10"},   {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "add"},
            {TokenType::WALRUS, ":="},   {TokenType::FUNCTION, "fn"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "x"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "y"},
            {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},
            {TokenType::IDENT, "x"},     {TokenType::PLUS, "+"},
            {TokenType::IDENT, "y"},     {TokenType::SEMICOLON, ";"},
            {TokenType::RBRACE, "}"},    {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "result"},
            {TokenType::WALRUS, ":="},   {TokenType::IDENT, "add"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "five"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "ten"},
            {TokenType::RPAREN, ")"},    {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "four_and_some"},
            {TokenType::WALRUS, ":="},   {TokenType::FLOAT, "4.2"},
            {TokenType::SEMICOLON, ";"}, {TokenType::END, ""},
        });

        Lexer      l_accumulator{input};
        const auto accumulated_tokens = l_accumulator.consume();
        l_accumulator.reset(input);
        const auto reset_acc = l_accumulator.consume();

        for (size_t i = 0; i < expecteds.size(); ++i) {
            const auto& [expected_tok, expected_slice] = expecteds[i];
            const auto token                           = l.advance();
            const auto accumulated_token               = accumulated_tokens[i];
            const auto reset                           = reset_acc[i];

            REQUIRE(expected_tok == token.type);
            REQUIRE(expected_tok == accumulated_token.type);
            REQUIRE(expected_slice == token.slice);
            REQUIRE(expected_slice == accumulated_token.slice);
            REQUIRE(accumulated_token == reset);
        }
    }
}

TEST_CASE("Base-10 ints and floats") {
    SECTION("Correct numbers") {
        Lexer l{"0 123 3.14 42.0 1e20 1.e-3 2.3901E4 1e."};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::INT_10, "0"},
            {TokenType::INT_10, "123"},
            {TokenType::FLOAT, "3.14"},
            {TokenType::FLOAT, "42.0"},
            {TokenType::FLOAT, "1e20"},
            {TokenType::FLOAT, "1.e-3"},
            {TokenType::FLOAT, "2.3901E4"},
            {TokenType::INT_10, "1"},
            {TokenType::IDENT, "e"},
            {TokenType::DOT, "."},
            {TokenType::END, ""},
        });

        for (const auto& [expected_tt, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tt == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Illegal Floats") {
        Lexer l{".0 1..2 3.4.5 3.4u"};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::DOT, "."},
            {TokenType::INT_10, "0"},
            {TokenType::INT_10, "1"},
            {TokenType::DOT_DOT, ".."},
            {TokenType::INT_10, "2"},
            {TokenType::FLOAT, "3.4"},
            {TokenType::DOT, "."},
            {TokenType::INT_10, "5"},
            {TokenType::FLOAT, "3.4"},
            {TokenType::IDENT, "u"},
            {TokenType::END, ""},
        });

        for (const auto& [expected_tt, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tt == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }
}

TEST_CASE("Integer base variants") {
    SECTION("Signed integer variants") {
        Lexer l{"0b1010 0o17 0O17 42 0x2A 0X2A 0b 0x 0o"};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::INT_2, "0b1010"},
            {TokenType::INT_8, "0o17"},
            {TokenType::INT_8, "0O17"},
            {TokenType::INT_10, "42"},
            {TokenType::INT_16, "0x2A"},
            {TokenType::INT_16, "0X2A"},
            {TokenType::ILLEGAL, "0b"},
            {TokenType::ILLEGAL, "0x"},
            {TokenType::ILLEGAL, "0o"},
            {TokenType::END, ""},
        });

        for (const auto& [expected_tt, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tt == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Unsigned integer variants") {
        Lexer l{"0b1010u 0b1010uz 0o17u 0o17uz 0O17u 42u 42UZ 0x2AU 0X2Au 123ufoo 0bu 0xu 0ou"};

        const auto expecteds = std::to_array<ExpectedLexeme>({
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
            {TokenType::END, ""},
        });

        for (const auto& [expected_tt, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tt == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }

    SECTION("Different integer widths") {
        Lexer l{"2 2l 2z 2u 2ul 2uz"};

        const auto expecteds = std::to_array<ExpectedLexeme>({
            {TokenType::INT_10, "2"},
            {TokenType::LINT_10, "2l"},
            {TokenType::ZINT_10, "2z"},
            {TokenType::UINT_10, "2u"},
            {TokenType::ULINT_10, "2ul"},
            {TokenType::UZINT_10, "2uz"},
        });

        for (const auto& [expected_tt, expected_slice] : expecteds) {
            const auto token = l.advance();
            REQUIRE(expected_tt == token.type);
            REQUIRE(expected_slice == token.slice);
        }
    }
}

TEST_CASE("Iterator with other keywords") {
    Lexer l{"private extern export packed volatile static "
            "int long isize uint ulong usize float byte string bool void type"};

    const auto expecteds = std::to_array<ExpectedLexeme>({
        {TokenType::PRIVATE, "private"},
        {TokenType::EXTERN, "extern"},
        {TokenType::EXPORT, "export"},
        {TokenType::PACKED, "packed"},
        {TokenType::VOLATILE, "volatile"},
        {TokenType::STATIC, "static"},
        {TokenType::INT_TYPE, "int"},
        {TokenType::LONG_TYPE, "long"},
        {TokenType::ISIZE_TYPE, "isize"},
        {TokenType::UINT_TYPE, "uint"},
        {TokenType::ULONG_TYPE, "ulong"},
        {TokenType::USIZE_TYPE, "usize"},
        {TokenType::FLOAT_TYPE, "float"},
        {TokenType::BYTE_TYPE, "byte"},
        {TokenType::STRING_TYPE, "string"},
        {TokenType::BOOL_TYPE, "bool"},
        {TokenType::VOID_TYPE, "void"},
        {TokenType::TYPE_TYPE, "type"},
    });

    size_t i = 0;
    for (const auto& token : l) {
        REQUIRE(expecteds[i].first == token.type);
        REQUIRE(expecteds[i].second == token.slice);
        i += 1;
    }
}

TEST_CASE("Comments") {
    Lexer l{"const five := 5;\n"
            "var ten_10 := 10;\n\n"
            "// BOL\n"
            "var add := fn(x, y) {\n"
            "   x + y;\n"
            "};\n\n"
            "var result := add(five, ten); // EOL\n"
            "var four_and_some := 4.2;\n"
            "work;"};

    const auto expecteds = std::to_array<ExpectedLexeme>({
        {TokenType::CONST, "const"},  {TokenType::IDENT, "five"},
        {TokenType::WALRUS, ":="},    {TokenType::INT_10, "5"},
        {TokenType::SEMICOLON, ";"},  {TokenType::VAR, "var"},
        {TokenType::IDENT, "ten_10"}, {TokenType::WALRUS, ":="},
        {TokenType::INT_10, "10"},    {TokenType::SEMICOLON, ";"},
        {TokenType::COMMENT, " BOL"}, {TokenType::VAR, "var"},
        {TokenType::IDENT, "add"},    {TokenType::WALRUS, ":="},
        {TokenType::FUNCTION, "fn"},  {TokenType::LPAREN, "("},
        {TokenType::IDENT, "x"},      {TokenType::COMMA, ","},
        {TokenType::IDENT, "y"},      {TokenType::RPAREN, ")"},
        {TokenType::LBRACE, "{"},     {TokenType::IDENT, "x"},
        {TokenType::PLUS, "+"},       {TokenType::IDENT, "y"},
        {TokenType::SEMICOLON, ";"},  {TokenType::RBRACE, "}"},
        {TokenType::SEMICOLON, ";"},  {TokenType::VAR, "var"},
        {TokenType::IDENT, "result"}, {TokenType::WALRUS, ":="},
        {TokenType::IDENT, "add"},    {TokenType::LPAREN, "("},
        {TokenType::IDENT, "five"},   {TokenType::COMMA, ","},
        {TokenType::IDENT, "ten"},    {TokenType::RPAREN, ")"},
        {TokenType::SEMICOLON, ";"},  {TokenType::COMMENT, " EOL"},
        {TokenType::VAR, "var"},      {TokenType::IDENT, "four_and_some"},
        {TokenType::WALRUS, ":="},    {TokenType::FLOAT, "4.2"},
        {TokenType::SEMICOLON, ";"},  {TokenType::IDENT, "work"},
        {TokenType::SEMICOLON, ";"},  {TokenType::END, ""},
    });

    for (const auto& [expected_tt, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_tt == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("Character literals") {
    Lexer l{"if'e' else'\\'\nreturn'\\r' break'\\n'\n"
            "continue'\\0' for'\\'' while'\\\\' const''\n"
            "var'asd'"};

    const auto expecteds = std::to_array<ExpectedLexeme>({
        {TokenType::IF, "if"},
        {TokenType::BYTE, "'e'"},
        {TokenType::ELSE, "else"},
        {TokenType::ILLEGAL, "'\\'"},
        {TokenType::RETURN, "return"},
        {TokenType::BYTE, "'\\r'"},
        {TokenType::BREAK, "break"},
        {TokenType::BYTE, "'\\n'"},
        {TokenType::CONTINUE, "continue"},
        {TokenType::BYTE, "'\\0'"},
        {TokenType::FOR, "for"},
        {TokenType::BYTE, "'\\''"},
        {TokenType::WHILE, "while"},
        {TokenType::BYTE, "'\\\\'"},
        {TokenType::CONST, "const"},
        {TokenType::ILLEGAL, "'"},
        {TokenType::ILLEGAL, "'"},
        {TokenType::VAR, "var"},
        {TokenType::ILLEGAL, "'asd'"},
        {TokenType::END, ""},
    });

    for (const auto& [expected_tt, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_tt == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("String literals") {
    Lexer l{
        R"("This is a string";const five := "Hello, World!";var ten: string = "Hello\n, World!\0";var one := "Hello, World!;)"};

    const auto expecteds = std::to_array<ExpectedLexeme>({
        {TokenType::STRING, R"("This is a string")"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::CONST, "const"},
        {TokenType::IDENT, "five"},
        {TokenType::WALRUS, ":="},
        {TokenType::STRING, R"("Hello, World!")"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::VAR, "var"},
        {TokenType::IDENT, "ten"},
        {TokenType::COLON, ":"},
        {TokenType::STRING_TYPE, "string"},
        {TokenType::ASSIGN, "="},
        {TokenType::STRING, R"("Hello\n, World!\0")"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::VAR, "var"},
        {TokenType::IDENT, "one"},
        {TokenType::WALRUS, ":="},
        {TokenType::ILLEGAL, R"("Hello, World!;)"},
        {TokenType::END, ""},
    });

    for (const auto& [expected_tt, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_tt == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("Multiline string literals") {
    Lexer l{"const five := \\\\Multiline stringing\n"
            ";\n"
            "var ten := \\\\Multiline stringing\n"
            "\\\\Continuation\n"
            ";\n"
            "const one := \\\\Nesting \" \' \\ [] const var\n"
            "\\\\\n"
            ";\n"};

    const auto expecteds = std::to_array<ExpectedLexeme>({
        {TokenType::CONST, "const"},
        {TokenType::IDENT, "five"},
        {TokenType::WALRUS, ":="},
        {TokenType::MULTILINE_STRING, "Multiline stringing"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::VAR, "var"},
        {TokenType::IDENT, "ten"},
        {TokenType::WALRUS, ":="},
        {TokenType::MULTILINE_STRING, "Multiline stringing\n\\\\Continuation"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::CONST, "const"},
        {TokenType::IDENT, "one"},
        {TokenType::WALRUS, ":="},
        {TokenType::MULTILINE_STRING, "Nesting \" \' \\ [] const var\n\\\\"},
        {TokenType::SEMICOLON, ";"},
        {TokenType::END, ""},
    });

    for (const auto& [expected_tt, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_tt == token.type);
        REQUIRE(expected_slice == token.slice);
    }
}

TEST_CASE("Compiler builtins") {
    const auto expecteds =
        std::ranges::views::transform(ALL_BUILTINS, [](const auto& builtin) -> ExpectedLexeme {
            return {builtin.second, builtin.first};
        });

    std::string input;
    std::ranges::for_each(ALL_BUILTINS, [&input](const auto& builtin) -> void {
        input.append(builtin.first);
        input.push_back(' ');
    });
    Lexer l{input};

    for (const auto& [expected_tt, expected_slice] : expecteds) {
        const auto token = l.advance();
        REQUIRE(expected_tt == token.type);
        REQUIRE(expected_slice == token.slice);
        REQUIRE(is_builtin(token.type));
    }
}

TEST_CASE("Illegal builtins") {
    const std::string_view input{"@run"};
    Lexer                  l{input};
    const auto             illegal = l.advance();
    REQUIRE(TokenType::ILLEGAL == illegal.type);
    REQUIRE(input == illegal.slice);
}

} // namespace conch::tests
