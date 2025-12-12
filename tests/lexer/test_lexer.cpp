#include "catch_amalgamated.hpp"

#include <stdio.h>
#include <string.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "file_helpers.hpp"

extern "C" {
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "util/allocator.h"
#include "util/io.h"
#include "util/mem.h"
#include "util/status.h"
}

using ExpectedToken = std::pair<TokenType, const char*>;

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Symbols Only") {
        const char* input = "=+(){}[],;: !-/*<>_";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::ASSIGN, "="},    {TokenType::PLUS, "+"},     {TokenType::LPAREN, "("},
            {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},   {TokenType::RBRACE, "}"},
            {TokenType::LBRACKET, "["},  {TokenType::RBRACKET, "]"}, {TokenType::COMMA, ","},
            {TokenType::SEMICOLON, ";"}, {TokenType::COLON, ":"},    {TokenType::BANG, "!"},
            {TokenType::MINUS, "-"},     {TokenType::SLASH, "/"},    {TokenType::STAR, "*"},
            {TokenType::LT, "<"},        {TokenType::GT, ">"},       {TokenType::UNDERSCORE, "_"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);

            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }

        lexer_deinit(&l);
    }

    SECTION("Basic Language Snippet") {
        const char* input = "const five = 5;\n"
                            "var ten = 10;\n\n"
                            "var add = fn(x, y) {\n"
                            "   x + y;\n"
                            "};\n\n"
                            "var result = add(five, ten);\n"
                            "var four_and_some = 4.2;";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::CONST, "const"}, {TokenType::IDENT, "five"},
            {TokenType::ASSIGN, "="},    {TokenType::INT_10, "5"},
            {TokenType::SEMICOLON, ";"}, {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},   {TokenType::ASSIGN, "="},
            {TokenType::INT_10, "10"},   {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "add"},
            {TokenType::ASSIGN, "="},    {TokenType::FUNCTION, "fn"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "x"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "y"},
            {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},
            {TokenType::IDENT, "x"},     {TokenType::PLUS, "+"},
            {TokenType::IDENT, "y"},     {TokenType::SEMICOLON, ";"},
            {TokenType::RBRACE, "}"},    {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "result"},
            {TokenType::ASSIGN, "="},    {TokenType::IDENT, "add"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "five"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "ten"},
            {TokenType::RPAREN, ")"},    {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},     {TokenType::IDENT, "four_and_some"},
            {TokenType::ASSIGN, "="},    {TokenType::FLOAT, "4.2"},
            {TokenType::SEMICOLON, ";"}, {TokenType::END, ""},
        };

        Lexer l_accumulator;
        REQUIRE(STATUS_OK(lexer_init(&l_accumulator, input, standard_allocator)));
        REQUIRE(STATUS_OK(lexer_consume(&l_accumulator)));
        ArrayList* accumulated_tokens = &l_accumulator.token_accumulator;

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            Token accumulated_token;
            REQUIRE(STATUS_OK(array_list_get(accumulated_tokens, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }

        lexer_deinit(&l);
        lexer_deinit(&l_accumulator);
    }
}

TEST_CASE("Numbers and lexer consumer resets") {
    Lexer reseting_lexer;
    REQUIRE(STATUS_OK(lexer_null_init(&reseting_lexer, standard_allocator)));

    FileIO dbgio;
    REQUIRE(STATUS_OK(file_io_init(&dbgio, stdin, stdout, stderr)));

    SECTION("Correct base-10 ints and floats") {
        const char* input = "0 123 3.14 42.0 1e20 1.e-3 2.3901E4 1e.";

        std::vector<ExpectedToken> expecteds = {
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
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            Token accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Signed integer variants") {
        const char* input = "0b1010 0o17 0O17 42 0x2A 0X2A 0b 0x 0o";

        std::vector<ExpectedToken> expecteds = {
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
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            Token accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Unsigned integer variants") {
        const char* input = "0b1010u 0o17u 0O17u 42u 0x2AU 0X2Au 123ufoo 0bu 0xu 0ou";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::UINT_2, "0b1010u"},
            {TokenType::UINT_8, "0o17u"},
            {TokenType::UINT_8, "0O17u"},
            {TokenType::UINT_10, "42u"},
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
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            Token accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Illegal Floats") {
        const char* input = ".0 1..2 3.4.5 3.4u";

        std::vector<ExpectedToken> expecteds = {
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
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            Token accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_deinit(&l);
    }

    lexer_deinit(&reseting_lexer);
}

TEST_CASE("Advanced next token") {
    SECTION("Keywords") {
        const char* input = "struct true false and or orelse enum nil is int uint float impl";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::STRUCT, "struct"},
            {TokenType::TRUE, "true"},
            {TokenType::FALSE, "false"},
            {TokenType::BOOLEAN_AND, "and"},
            {TokenType::BOOLEAN_OR, "or"},
            {TokenType::ORELSE, "orelse"},
            {TokenType::ENUM, "enum"},
            {TokenType::NIL, "nil"},
            {TokenType::IS, "is"},
            {TokenType::INT_TYPE, "int"},
            {TokenType::UINT_TYPE, "uint"},
            {TokenType::FLOAT_TYPE, "float"},
            {TokenType::IMPL, "impl"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("General operators") {
        const char* input = ":= + += - -= * *= / /= & &= | |= << <<= >> >>= ~ ~= % %=";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::WALRUS, ":="},         {TokenType::PLUS, "+"},
            {TokenType::PLUS_ASSIGN, "+="},    {TokenType::MINUS, "-"},
            {TokenType::MINUS_ASSIGN, "-="},   {TokenType::STAR, "*"},
            {TokenType::STAR_ASSIGN, "*="},    {TokenType::SLASH, "/"},
            {TokenType::SLASH_ASSIGN, "/="},   {TokenType::AND, "&"},
            {TokenType::AND_ASSIGN, "&="},     {TokenType::OR, "|"},
            {TokenType::OR_ASSIGN, "|="},      {TokenType::SHL, "<<"},
            {TokenType::SHL_ASSIGN, "<<="},    {TokenType::SHR, ">>"},
            {TokenType::SHR_ASSIGN, ">>="},    {TokenType::NOT, "~"},
            {TokenType::NOT_ASSIGN, "~="},     {TokenType::PERCENT, "%"},
            {TokenType::PERCENT_ASSIGN, "%="}, {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Dot operators") {
        const char* input = ". .. ..= : ::";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::DOT, "."},
            {TokenType::DOT_DOT, ".."},
            {TokenType::DOT_DOT_EQ, "..="},
            {TokenType::COLON, ":"},
            {TokenType::COLON_COLON, "::"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Control flow keywords") {
        const char* input = "if else match return for while do continue break with";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::IF, "if"},
            {TokenType::ELSE, "else"},
            {TokenType::MATCH, "match"},
            {TokenType::RETURN, "return"},
            {TokenType::FOR, "for"},
            {TokenType::WHILE, "while"},
            {TokenType::DO, "do"},
            {TokenType::CONTINUE, "continue"},
            {TokenType::BREAK, "break"},
            {TokenType::WITH, "with"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }
}

TEST_CASE("TokenType regression guard") {
    REQUIRE(TokenType::END == 0);

    const size_t illegal_index   = (size_t)ILLEGAL;
    const size_t name_array_size = sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0]);
    REQUIRE(illegal_index == (name_array_size - 1));

    for (size_t i = 0; i < name_array_size; i++) {
        const char* name = token_type_name((TokenType)i);
        REQUIRE(strcmp(TOKEN_TYPE_NAMES[i], name) == 0);
    }
}

TEST_CASE("Advanced literals") {
    SECTION("Comment literals") {
        const char* input = "const five = 5;\n"
                            "var ten = 10;\n\n"
                            "// Comment on a new line\n"
                            "var add = fn(x, y) {\n"
                            "   x + y;\n"
                            "};\n\n"
                            "var result = add(five, ten); // This is an end of line comment\n"
                            "var four_and_some = 4.2;\n"
                            "work;";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::CONST, "const"},
            {TokenType::IDENT, "five"},
            {TokenType::ASSIGN, "="},
            {TokenType::INT_10, "5"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},
            {TokenType::ASSIGN, "="},
            {TokenType::INT_10, "10"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::COMMENT, " Comment on a new line"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "add"},
            {TokenType::ASSIGN, "="},
            {TokenType::FUNCTION, "fn"},
            {TokenType::LPAREN, "("},
            {TokenType::IDENT, "x"},
            {TokenType::COMMA, ","},
            {TokenType::IDENT, "y"},
            {TokenType::RPAREN, ")"},
            {TokenType::LBRACE, "{"},
            {TokenType::IDENT, "x"},
            {TokenType::PLUS, "+"},
            {TokenType::IDENT, "y"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::RBRACE, "}"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "result"},
            {TokenType::ASSIGN, "="},
            {TokenType::IDENT, "add"},
            {TokenType::LPAREN, "("},
            {TokenType::IDENT, "five"},
            {TokenType::COMMA, ","},
            {TokenType::IDENT, "ten"},
            {TokenType::RPAREN, ")"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::COMMENT, " This is an end of line comment"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "four_and_some"},
            {TokenType::ASSIGN, "="},
            {TokenType::FLOAT, "4.2"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::IDENT, "work"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(&l);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }

        lexer_deinit(&l);
    }

    SECTION("Character literals") {
        const char* input = "if'e' else'\\'\nreturn'\\r' break'\\n'\n"
                            "continue'\\0' for'\\'' while'\\\\' const''\n"
                            "var'asd'";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::IF, "if"},
            {TokenType::CHARACTER, "'e'"},
            {TokenType::ELSE, "else"},
            {TokenType::ILLEGAL, "'\\'"},
            {TokenType::RETURN, "return"},
            {TokenType::CHARACTER, "'\\r'"},
            {TokenType::BREAK, "break"},
            {TokenType::CHARACTER, "'\\n'"},
            {TokenType::CONTINUE, "continue"},
            {TokenType::CHARACTER, "'\\0'"},
            {TokenType::FOR, "for"},
            {TokenType::CHARACTER, "'\\''"},
            {TokenType::WHILE, "while"},
            {TokenType::CHARACTER, "'\\\\'"},
            {TokenType::CONST, "const"},
            {TokenType::ILLEGAL, "'"},
            {TokenType::ILLEGAL, "'"},
            {TokenType::VAR, "var"},
            {TokenType::ILLEGAL, "'asd'"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("String literals") {
        const char* input = "const five = \"Hello, World!\";\n"
                            "var ten = \"Hello\\n, World!\\0\";"
                            "var one := \"Hello, World!;";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::CONST, "const"},
            {TokenType::IDENT, "five"},
            {TokenType::ASSIGN, "="},
            {TokenType::STRING, "\"Hello, World!\""},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},
            {TokenType::ASSIGN, "="},
            {TokenType::STRING, "\"Hello\\n, World!\\0\""},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "one"},
            {TokenType::WALRUS, ":="},
            {TokenType::ILLEGAL, "\"Hello, World!;"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }

        lexer_deinit(&l);
    }

    SECTION("Multiline string literals") {
        const char* input = "const five = \\\\Multiline stringing\n"
                            ";\n"
                            "var ten = \\\\Multiline stringing\n"
                            "\\\\Continuation\n"
                            ";\n"
                            "const one = \\\\Nesting \" \' \\ [] const var\n"
                            "\\\\\n"
                            ";\n";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::CONST, "const"},
            {TokenType::IDENT, "five"},
            {TokenType::ASSIGN, "="},
            {TokenType::MULTILINE_STRING, "Multiline stringing"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::VAR, "var"},
            {TokenType::IDENT, "ten"},
            {TokenType::ASSIGN, "="},
            {TokenType::MULTILINE_STRING, "Multiline stringing\n\\\\Continuation"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::CONST, "const"},
            {TokenType::IDENT, "one"},
            {TokenType::ASSIGN, "="},
            {TokenType::MULTILINE_STRING, "Nesting \" \' \\ [] const var\n\\\\"},
            {TokenType::SEMICOLON, ";"},
            {TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_deinit(&l);
    }

    SECTION("Promotion of invalid tokens") {
        Token    string_tok = token_init(TokenType::INT_10, "1", strlen("1"), 0, 0);
        MutSlice promoted_string;
        REQUIRE(promote_token_string(string_tok, &promoted_string, standard_allocator) ==
                Status::TYPE_MISMATCH);
    }

    SECTION("Promotion of standard string literals") {
        SECTION("Normal case") {
            Token string_tok = token_init(
                TokenType::STRING, "\"Hello, World!\"", strlen("\"Hello, World!\""), 0, 0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "Hello, World!");
            free(promoted_string.ptr);
        }

        SECTION("Escaped case") {
            Token string_tok = token_init(
                TokenType::STRING, "\"\"Hello, World!\"\"", strlen("\"\"Hello, World!\"\""), 0, 0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "\"Hello, World!\"");
            free(promoted_string.ptr);
        }

        SECTION("Empty case") {
            Token    string_tok = token_init(TokenType::STRING, "\"\"", strlen("\"\""), 0, 0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "");
            free(promoted_string.ptr);
        }

        SECTION("Malformed case") {
            Token    string_tok = token_init(TokenType::STRING, "\"", strlen("\""), 0, 0);
            MutSlice promoted_string;
            REQUIRE(promote_token_string(string_tok, &promoted_string, standard_allocator) ==
                    Status::UNEXPECTED_TOKEN);
        }
    }

    SECTION("Promotion of multistring literals") {
        SECTION("Normal case no newline") {
            Token    string_tok = token_init(TokenType::MULTILINE_STRING,
                                          "\\\\Hello,\"World!\"",
                                          strlen("\\\\Hello,\"World!\""),
                                          0,
                                          0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "Hello,\"World!\"");
            free(promoted_string.ptr);
        }

        SECTION("Normal case newline") {
            Token    string_tok = token_init(TokenType::MULTILINE_STRING,
                                          "\\\\Hello,\n\\\\World!\n\\\\",
                                          strlen("\\\\Hello,\n\\\\World!\n\\\\"),
                                          0,
                                          0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "Hello,\nWorld!\n");
            free(promoted_string.ptr);
        }

        SECTION("Empty case") {
            Token    string_tok = token_init(TokenType::MULTILINE_STRING, "", strlen(""), 0, 0);
            MutSlice promoted_string;
            REQUIRE(
                STATUS_OK(promote_token_string(string_tok, &promoted_string, standard_allocator)));
            REQUIRE(promoted_string.ptr);
            mut_slice_equals_str_z(&promoted_string, "");
            free(promoted_string.ptr);
        }
    }
}

TEST_CASE("Token dumping") {
    const char* input = "true false and or orelse";
    Lexer       l;
    REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
    REQUIRE(STATUS_OK(lexer_consume(&l)));

    TempFile temp_out("TMP_token_dump_out"), temp_err("TMP_token_dump_err");

    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO io;
    REQUIRE(STATUS_OK(file_io_init(&io, NULL, out, err)));

    REQUIRE(STATUS_OK(lexer_print_tokens(&l, &io)));
    std::ifstream out_fs(temp_out.path(), std::ios::binary);
    std::string   captured_out((std::istreambuf_iterator<char>(out_fs)),
                             std::istreambuf_iterator<char>());

    std::vector<std::string> expected_lines = {
        "TRUE(true) [1, 1]",
        "FALSE(false) [1, 6]",
        "BOOLEAN_AND(and) [1, 12]",
        "BOOLEAN_OR(or) [1, 16]",
        "ORELSE(orelse) [1, 19]",
        "END() [1, 25]",
    };

    std::vector<std::string> actual_lines;
    std::istringstream       stream(captured_out);
    std::string              line;

    while (std::getline(stream, line)) {
        if (line.length() > 0) {
            actual_lines.push_back(line);
        }
    }

    REQUIRE(expected_lines.size() == actual_lines.size());
    for (size_t i = 0; i < expected_lines.size(); i++) {
        const auto& expected = expected_lines[i];
        const auto& actual   = actual_lines[i];
        REQUIRE(expected == actual);
    }

    lexer_deinit(&l);
    file_io_deinit(&io);
}
