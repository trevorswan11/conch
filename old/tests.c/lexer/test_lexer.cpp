#include "catch_amalgamated.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "file_io.hpp"
#include "fixtures.hpp"
#include "util/containers/array_list.h"

extern "C" {
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "util/io.h"
#include "util/memory.h"
#include "util/status.h"
}

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Highly illegal characters") {
        const char* input = "月😭🎶";

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        REQUIRE(STATUS_OK(lexer_consume(&l)));
        const ArrayList* accumulated_tokens = &l.token_accumulator;
        auto             it                 = array_list_const_iterator_init(accumulated_tokens);

        Token token;
        while (array_list_const_iterator_has_next(&it, &token)) {
            if (array_list_const_iterator_exhausted(&it)) {
                REQUIRE(token.type == TokenType::END);
                break;
            }
            REQUIRE(token.type == TokenType::ILLEGAL);
        }
    }

    SECTION("Symbols Only") {
        const char* input = "=+(){}[],;: !-/*<>_";

        const auto expecteds = std::array{
            std::pair{TokenType::ASSIGN, "="},   std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::LPAREN, "("},   std::pair{TokenType::RPAREN, ")"},
            std::pair{TokenType::LBRACE, "{"},   std::pair{TokenType::RBRACE, "}"},
            std::pair{TokenType::LBRACKET, "["}, std::pair{TokenType::RBRACKET, "]"},
            std::pair{TokenType::COMMA, ","},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::COLON, ":"},    std::pair{TokenType::BANG, "!"},
            std::pair{TokenType::MINUS, "-"},    std::pair{TokenType::SLASH, "/"},
            std::pair{TokenType::STAR, "*"},     std::pair{TokenType::LT, "<"},
            std::pair{TokenType::GT, ">"},       std::pair{TokenType::UNDERSCORE, "_"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("Basic Language Snippet") {
        const char* input = "const five = 5;\n"
                            "var ten = 10;\n\n"
                            "var add = fn(x, y) {\n"
                            "   x + y;\n"
                            "};\n\n"
                            "var result = add(five, ten);\n"
                            "var four_and_some = 4.2;";

        const auto expecteds = std::array{
            std::pair{TokenType::CONST, "const"}, std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::INT_10, "5"},
            std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "ten"},   std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::INT_10, "10"},   std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FUNCTION, "fn"},
            std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "x"},
            std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "y"},
            std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::LBRACE, "{"},
            std::pair{TokenType::IDENT, "x"},     std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::IDENT, "y"},     std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::RBRACE, "}"},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "result"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::LPAREN, "("},    std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::COMMA, ","},     std::pair{TokenType::IDENT, "ten"},
            std::pair{TokenType::RPAREN, ")"},    std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},     std::pair{TokenType::IDENT, "four_and_some"},
            std::pair{TokenType::ASSIGN, "="},    std::pair{TokenType::FLOAT, "4.2"},
            std::pair{TokenType::SEMICOLON, ";"}, std::pair{TokenType::END, ""},
        };

        Lexer l_accumulator;
        REQUIRE(STATUS_OK(lexer_init(&l_accumulator, input, &std_allocator)));
        const Fixture<Lexer> lfa{l_accumulator, lexer_deinit};
        REQUIRE(STATUS_OK(lexer_consume(&l_accumulator)));
        const ArrayList* accumulated_tokens = &l_accumulator.token_accumulator;

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lfb{l, lexer_deinit};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            const auto token   = lexer_next_token(&l);
            Token      accumulated_token;
            REQUIRE(STATUS_OK(array_list_get(accumulated_tokens, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
    }
}

TEST_CASE("Numbers and lexer consumer resets") {
    Lexer reseting_lexer;
    REQUIRE(STATUS_OK(lexer_null_init(&reseting_lexer, &std_allocator)));
    const Fixture<Lexer> lfr{reseting_lexer, lexer_deinit};

    SECTION("Correct base-10 ints and floats") {
        const char* input = "0 123 3.14 42.0 1e20 1.e-3 2.3901E4 1e.";

        const auto expecteds = std::array{
            std::pair{TokenType::INT_10, "0"},
            std::pair{TokenType::INT_10, "123"},
            std::pair{TokenType::FLOAT, "3.14"},
            std::pair{TokenType::FLOAT, "42.0"},
            std::pair{TokenType::FLOAT, "1e20"},
            std::pair{TokenType::FLOAT, "1.e-3"},
            std::pair{TokenType::FLOAT, "2.3901E4"},
            std::pair{TokenType::INT_10, "1"},
            std::pair{TokenType::IDENT, "e"},
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::END, ""},
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            const auto token   = lexer_next_token(&l);
            Token      accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
    }

    SECTION("Signed integer variants") {
        const char* input = "0b1010 0o17 0O17 42 0x2A 0X2A 0b 0x 0o";

        const auto expecteds = std::array{
            std::pair{TokenType::INT_2, "0b1010"},
            std::pair{TokenType::INT_8, "0o17"},
            std::pair{TokenType::INT_8, "0O17"},
            std::pair{TokenType::INT_10, "42"},
            std::pair{TokenType::INT_16, "0x2A"},
            std::pair{TokenType::INT_16, "0X2A"},
            std::pair{TokenType::ILLEGAL, "0b"},
            std::pair{TokenType::ILLEGAL, "0x"},
            std::pair{TokenType::ILLEGAL, "0o"},
            std::pair{TokenType::END, ""},
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            const auto token   = lexer_next_token(&l);
            Token      accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
    }

    SECTION("Unsigned integer variants") {
        const char* input =
            "0b1010u 0b1010uz 0o17u 0o17uz 0O17u 42u 42UZ 0x2AU 0X2Au 123ufoo 0bu 0xu 0ou";

        const auto expecteds = std::array{
            std::pair{TokenType::UINT_2, "0b1010u"},
            std::pair{TokenType::UZINT_2, "0b1010uz"},
            std::pair{TokenType::UINT_8, "0o17u"},
            std::pair{TokenType::UZINT_8, "0o17uz"},
            std::pair{TokenType::UINT_8, "0O17u"},
            std::pair{TokenType::UINT_10, "42u"},
            std::pair{TokenType::UZINT_10, "42UZ"},
            std::pair{TokenType::UINT_16, "0x2AU"},
            std::pair{TokenType::UINT_16, "0X2Au"},
            std::pair{TokenType::UINT_10, "123u"},
            std::pair{TokenType::IDENT, "foo"},
            std::pair{TokenType::ILLEGAL, "0b"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::ILLEGAL, "0x"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::ILLEGAL, "0o"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::END, ""},
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            const auto token   = lexer_next_token(&l);
            Token      accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
    }

    SECTION("Illegal Floats") {
        const char* input = ".0 1..2 3.4.5 3.4u";

        const auto expecteds = std::array{
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::INT_10, "0"},
            std::pair{TokenType::INT_10, "1"},
            std::pair{TokenType::DOT_DOT, ".."},
            std::pair{TokenType::INT_10, "2"},
            std::pair{TokenType::FLOAT, "3.4"},
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::INT_10, "5"},
            std::pair{TokenType::FLOAT, "3.4"},
            std::pair{TokenType::IDENT, "u"},
            std::pair{TokenType::END, ""},
        };

        reseting_lexer.input        = input;
        reseting_lexer.input_length = strlen(input);
        REQUIRE(STATUS_OK(lexer_consume(&reseting_lexer)));

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            const auto token   = lexer_next_token(&l);
            Token      accumulated_token;
            REQUIRE(STATUS_OK(
                array_list_get(&reseting_lexer.token_accumulator, i, &accumulated_token)));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
    }
}

TEST_CASE("Advanced next token") {
    SECTION("Keywords") {
        const char* input = "struct true false and or orelse enum nil is int uint size float impl";

        const auto expecteds = std::array{
            std::pair{TokenType::STRUCT, "struct"},
            std::pair{TokenType::TRUE, "true"},
            std::pair{TokenType::FALSE, "false"},
            std::pair{TokenType::BOOLEAN_AND, "and"},
            std::pair{TokenType::BOOLEAN_OR, "or"},
            std::pair{TokenType::ORELSE, "orelse"},
            std::pair{TokenType::ENUM, "enum"},
            std::pair{TokenType::NIL, "nil"},
            std::pair{TokenType::IS, "is"},
            std::pair{TokenType::INT_TYPE, "int"},
            std::pair{TokenType::UINT_TYPE, "uint"},
            std::pair{TokenType::SIZE_TYPE, "size"},
            std::pair{TokenType::FLOAT_TYPE, "float"},
            std::pair{TokenType::IMPL, "impl"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("General operators") {
        const char* input = ":= + += - -= * *= / /= & &= | |= << <<= >> >>= ~ ~= % %=";

        const auto expecteds = std::array{
            std::pair{TokenType::WALRUS, ":="},         std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::PLUS_ASSIGN, "+="},    std::pair{TokenType::MINUS, "-"},
            std::pair{TokenType::MINUS_ASSIGN, "-="},   std::pair{TokenType::STAR, "*"},
            std::pair{TokenType::STAR_ASSIGN, "*="},    std::pair{TokenType::SLASH, "/"},
            std::pair{TokenType::SLASH_ASSIGN, "/="},   std::pair{TokenType::AND, "&"},
            std::pair{TokenType::AND_ASSIGN, "&="},     std::pair{TokenType::OR, "|"},
            std::pair{TokenType::OR_ASSIGN, "|="},      std::pair{TokenType::SHL, "<<"},
            std::pair{TokenType::SHL_ASSIGN, "<<="},    std::pair{TokenType::SHR, ">>"},
            std::pair{TokenType::SHR_ASSIGN, ">>="},    std::pair{TokenType::NOT, "~"},
            std::pair{TokenType::NOT_ASSIGN, "~="},     std::pair{TokenType::PERCENT, "%"},
            std::pair{TokenType::PERCENT_ASSIGN, "%="}, std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("Dot operators") {
        const char* input = ". .. ..= : ::";

        const auto expecteds = std::array{
            std::pair{TokenType::DOT, "."},
            std::pair{TokenType::DOT_DOT, ".."},
            std::pair{TokenType::DOT_DOT_EQ, "..="},
            std::pair{TokenType::COLON, ":"},
            std::pair{TokenType::COLON_COLON, "::"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("Control flow keywords") {
        const char* input = "if else match return for while do loop continue break with";

        const auto expecteds = std::array{
            std::pair{TokenType::IF, "if"},
            std::pair{TokenType::ELSE, "else"},
            std::pair{TokenType::MATCH, "match"},
            std::pair{TokenType::RETURN, "return"},
            std::pair{TokenType::FOR, "for"},
            std::pair{TokenType::WHILE, "while"},
            std::pair{TokenType::DO, "do"},
            std::pair{TokenType::LOOP, "loop"},
            std::pair{TokenType::CONTINUE, "continue"},
            std::pair{TokenType::BREAK, "break"},
            std::pair{TokenType::WITH, "with"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }
}

TEST_CASE("TokenType regression guard") {
    REQUIRE(TokenType::END == 0);

    const auto   illegal_index   = static_cast<size_t>(ILLEGAL);
    const size_t name_array_size = sizeof(TOKEN_TYPE_NAMES) / sizeof(TOKEN_TYPE_NAMES[0]);
    REQUIRE(illegal_index == (name_array_size - 1));

    for (size_t i = 0; i < name_array_size; i++) {
        const char* name = token_type_name(static_cast<TokenType>(i));
        REQUIRE(strcmp(TOKEN_TYPE_NAMES[i], name) == 0);
    }
}

TEST_CASE("Advanced literals") {
    SECTION("Comment literals") {
        const char* input = "const five = 5;\n"
                            "var ten_10 = 10;\n\n"
                            "// Comment on a new line\n"
                            "var add = fn(x, y) {\n"
                            "   x + y;\n"
                            "};\n\n"
                            "var result = add(five, ten); // This is an end of line comment\n"
                            "var four_and_some = 4.2;\n"
                            "work;";

        const auto expecteds = std::array{
            std::pair{TokenType::CONST, "const"},
            std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::INT_10, "5"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "ten_10"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::INT_10, "10"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::COMMENT, " Comment on a new line"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::FUNCTION, "fn"},
            std::pair{TokenType::LPAREN, "("},
            std::pair{TokenType::IDENT, "x"},
            std::pair{TokenType::COMMA, ","},
            std::pair{TokenType::IDENT, "y"},
            std::pair{TokenType::RPAREN, ")"},
            std::pair{TokenType::LBRACE, "{"},
            std::pair{TokenType::IDENT, "x"},
            std::pair{TokenType::PLUS, "+"},
            std::pair{TokenType::IDENT, "y"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::RBRACE, "}"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "result"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::IDENT, "add"},
            std::pair{TokenType::LPAREN, "("},
            std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::COMMA, ","},
            std::pair{TokenType::IDENT, "ten"},
            std::pair{TokenType::RPAREN, ")"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::COMMENT, " This is an end of line comment"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "four_and_some"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::FLOAT, "4.2"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::IDENT, "work"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (auto [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("Character literals") {
        const char* input = "if'e' else'\\'\nreturn'\\r' break'\\n'\n"
                            "continue'\\0' for'\\'' while'\\\\' const''\n"
                            "var'asd'";

        const auto expecteds = std::array{
            std::pair{TokenType::IF, "if"},
            std::pair{TokenType::CHARACTER, "'e'"},
            std::pair{TokenType::ELSE, "else"},
            std::pair{TokenType::ILLEGAL, "'\\'"},
            std::pair{TokenType::RETURN, "return"},
            std::pair{TokenType::CHARACTER, "'\\r'"},
            std::pair{TokenType::BREAK, "break"},
            std::pair{TokenType::CHARACTER, "'\\n'"},
            std::pair{TokenType::CONTINUE, "continue"},
            std::pair{TokenType::CHARACTER, "'\\0'"},
            std::pair{TokenType::FOR, "for"},
            std::pair{TokenType::CHARACTER, "'\\''"},
            std::pair{TokenType::WHILE, "while"},
            std::pair{TokenType::CHARACTER, "'\\\\'"},
            std::pair{TokenType::CONST, "const"},
            std::pair{TokenType::ILLEGAL, "'"},
            std::pair{TokenType::ILLEGAL, "'"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::ILLEGAL, "'asd'"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("String literals") {
        const char* input =
            R"(const five = "Hello, World!";var ten = "Hello\n, World!\0";var one := "Hello, World!;)";

        const auto expecteds = std::array{
            std::pair{TokenType::CONST, "const"},
            std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::STRING, R"("Hello, World!")"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "ten"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::STRING, R"("Hello\n, World!\0")"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "one"},
            std::pair{TokenType::WALRUS, ":="},
            std::pair{TokenType::ILLEGAL, R"("Hello, World!;)"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
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

        const auto expecteds = std::array{
            std::pair{TokenType::CONST, "const"},
            std::pair{TokenType::IDENT, "five"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::MULTILINE_STRING, "Multiline stringing"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::VAR, "var"},
            std::pair{TokenType::IDENT, "ten"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::MULTILINE_STRING, "Multiline stringing\n\\\\Continuation"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::CONST, "const"},
            std::pair{TokenType::IDENT, "one"},
            std::pair{TokenType::ASSIGN, "="},
            std::pair{TokenType::MULTILINE_STRING, "Nesting \" \' \\ [] const var\n\\\\"},
            std::pair{TokenType::SEMICOLON, ";"},
            std::pair{TokenType::END, ""},
        };

        Lexer l;
        REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
        const Fixture<Lexer> lf{l, lexer_deinit};

        for (const auto& [t, s] : expecteds) {
            const auto token = lexer_next_token(&l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
    }

    SECTION("Promotion of invalid tokens") {
        const auto token_slice = slice_from_str_z("1");
        const auto string_tok =
            token_init(TokenType::INT_10, token_slice.ptr, token_slice.length, 0, 0);
        MutSlice promoted_string;
        REQUIRE(promote_token_string(string_tok, &promoted_string, &std_allocator) ==
                Status::TYPE_MISMATCH);
    }

    SECTION("Promotion of standard string literals") {
        SECTION("Normal case") {
            const auto token_slice = slice_from_str_z(R"("Hello, World!")");
            const auto string_tok =
                token_init(TokenType::STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected = "Hello, World!";
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }

        SECTION("Escaped case") {
            const auto token_slice = slice_from_str_z(R"(""Hello, World!"")");
            const auto string_tok =
                token_init(TokenType::STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected = R"("Hello, World!")";
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }

        SECTION("Empty case") {
            const auto token_slice = slice_from_str_z(R"("")");
            const auto string_tok =
                token_init(TokenType::STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected;
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }

        SECTION("Malformed case") {
            const auto token_slice = slice_from_str_z(R"(")");
            const auto string_tok =
                token_init(TokenType::STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(promote_token_string(string_tok, &promoted_string, &std_allocator) ==
                    Status::UNEXPECTED_TOKEN);
        }
    }

    SECTION("Promotion of multistring literals") {
        SECTION("Normal case no newline") {
            const auto token_slice = slice_from_str_z(R"(\\Hello,"World!")");
            const auto string_tok =
                token_init(TokenType::MULTILINE_STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected = R"(Hello,"World!")";
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }

        SECTION("Normal case newline") {
            const auto token_slice = slice_from_str_z("\\\\Hello,\n\\\\World!\n\\\\");
            const auto string_tok =
                token_init(TokenType::MULTILINE_STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected = "Hello,\nWorld!\n";
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }

        SECTION("Empty case") {
            const auto token_slice = slice_from_str_z("");
            const auto string_tok =
                token_init(TokenType::MULTILINE_STRING, token_slice.ptr, token_slice.length, 0, 0);
            MutSlice promoted_string;
            REQUIRE(STATUS_OK(promote_token_string(string_tok, &promoted_string, &std_allocator)));
            const Fixture<char*> msf{promoted_string.ptr};

            const std::string expected;
            REQUIRE(promoted_string.ptr);
            REQUIRE(expected == promoted_string.ptr);
        }
    }
}

TEST_CASE("Token dumping") {
    const char* input = "true false and or orelse";
    Lexer       l;
    REQUIRE(STATUS_OK(lexer_init(&l, input, &std_allocator)));
    const Fixture<Lexer> lf{l, lexer_deinit};

    REQUIRE(STATUS_OK(lexer_consume(&l)));

    TempFile temp_out("TMP_token_dump_out");
    TempFile temp_err("TMP_token_dump_err");

    FILE* out = temp_out.open("wb");
    FILE* err = temp_err.open("wb");

    FileIO                io = file_io_init(nullptr, out, err);
    const Fixture<FileIO> fiof{io, file_io_deinit};

    REQUIRE(STATUS_OK(lexer_print_tokens(&l, &io)));
    std::ifstream     out_fs(temp_out.path(), std::ios::binary);
    const std::string captured_out((std::istreambuf_iterator<char>(out_fs)),
                                   std::istreambuf_iterator<char>());

    const auto expected_lines = std::to_array<std::string>({
        "TRUE(true) [1, 1]",
        "FALSE(false) [1, 6]",
        "BOOLEAN_AND(and) [1, 12]",
        "BOOLEAN_OR(or) [1, 16]",
        "ORELSE(orelse) [1, 19]",
        "END() [1, 25]",
    });

    std::vector<std::string> actual_lines;
    std::istringstream       stream(captured_out);
    std::string              line;

    while (std::getline(stream, line)) {
        if (!line.empty()) { actual_lines.push_back(line); }
    }

    REQUIRE(expected_lines.size() == actual_lines.size());
    for (size_t i = 0; i < expected_lines.size(); i++) {
        const auto& expected = expected_lines[i];
        const auto& actual   = actual_lines[i];
        REQUIRE(expected == actual);
    }
}
