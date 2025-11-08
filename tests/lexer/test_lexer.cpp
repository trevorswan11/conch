#include "catch_amalgamated.hpp"

#include <string.h>

#include <iostream>
#include <utility>
#include <vector>

extern "C" {
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "util/mem.h"
}

using ExpectedToken = std::pair<TokenType, const char*>;

TEST_CASE("Basic next token and lexer consuming") {
    SECTION("Symbols Only") {
        const char* input = "=+(){},; !-/*<>";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::ASSIGN, "="},
            {TokenType::PLUS, "+"},
            {TokenType::LPAREN, "("},
            {TokenType::RPAREN, ")"},
            {TokenType::LBRACE, "{"},
            {TokenType::RBRACE, "}"},
            {TokenType::COMMA, ","},
            {TokenType::SEMICOLON, ";"},
            {TokenType::BANG, "!"},
            {TokenType::MINUS, "-"},
            {TokenType::SLASH, "/"},
            {TokenType::STAR, "*"},
            {TokenType::LT, "<"},
            {TokenType::GT, ">"},
            {TokenType::END, ""},
        };

        Lexer* l = lexer_create(input);

        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(l);

            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }

        lexer_destroy(l);
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

        Lexer* l_accumulator = lexer_create(input);
        REQUIRE(lexer_consume(l_accumulator));
        ArrayList* accumulated_tokens = &l_accumulator->token_accumulator;

        Lexer* l = lexer_create(input);

        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(l);
            Token accumulated_token;
            REQUIRE(array_list_get(accumulated_tokens, i, &accumulated_token));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }

        lexer_destroy(l);
        lexer_destroy(l_accumulator);
    }
}

TEST_CASE("Numbers and lexer consumer resets") {
    Lexer* reseting_lexer = lexer_create("NULL");

    SECTION("Correct base-10 ints and floats") {
        const char* input = "0 123 3.14 42.0";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::INT_10, "0"},
            {TokenType::INT_10, "123"},
            {TokenType::FLOAT, "3.14"},
            {TokenType::FLOAT, "42.0"},
            {TokenType::END, ""},
        };

        reseting_lexer->input        = input;
        reseting_lexer->input_length = strlen(input);
        REQUIRE(lexer_consume(reseting_lexer));

        Lexer* l = lexer_create(input);
        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(l);
            Token accumulated_token;
            REQUIRE(array_list_get(&reseting_lexer->token_accumulator, i, &accumulated_token));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_destroy(l);
    }

    SECTION("Integer variants") {
        const char* input = "0b1010 0O17 42 0x2A 0b 0x 0o";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::INT_2, "0b1010"},
            {TokenType::INT_8, "0O17"},
            {TokenType::INT_10, "42"},
            {TokenType::INT_16, "0x2A"},
            {TokenType::ILLEGAL, "0b"},
            {TokenType::ILLEGAL, "0x"},
            {TokenType::ILLEGAL, "0o"},
            {TokenType::END, ""},
        };

        reseting_lexer->input        = input;
        reseting_lexer->input_length = strlen(input);
        REQUIRE(lexer_consume(reseting_lexer));

        Lexer* l = lexer_create(input);
        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(l);
            Token accumulated_token;
            REQUIRE(array_list_get(&reseting_lexer->token_accumulator, i, &accumulated_token));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_destroy(l);
    }

    SECTION("Illegal Floats") {
        const char* input = ".0 1..2 3.4.5";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::DOT, "."},
            {TokenType::INT_10, "0"},
            {TokenType::INT_10, "1"},
            {TokenType::DOT_DOT, ".."},
            {TokenType::INT_10, "2"},
            {TokenType::FLOAT, "3.4"},
            {TokenType::DOT, "."},
            {TokenType::INT_10, "5"},
            {TokenType::END, ""},
        };

        reseting_lexer->input        = input;
        reseting_lexer->input_length = strlen(input);
        REQUIRE(lexer_consume(reseting_lexer));

        Lexer* l = lexer_create(input);
        for (size_t i = 0; i < expecteds.size(); i++) {
            const auto& [t, s] = expecteds[i];
            Token token        = lexer_next_token(l);
            Token accumulated_token;
            REQUIRE(array_list_get(&reseting_lexer->token_accumulator, i, &accumulated_token));

            REQUIRE(t == token.type);
            REQUIRE(t == accumulated_token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
            REQUIRE(slice_equals_str_z(&accumulated_token.slice, s));
        }
        lexer_destroy(l);
    }

    lexer_destroy(reseting_lexer);
}

TEST_CASE("Advanced next token") {
    SECTION("Keywords") {
        const char* input = "struct true false and or enum nil is";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::STRUCT, "struct"},
            {TokenType::TRUE, "true"},
            {TokenType::FALSE, "false"},
            {TokenType::BOOLEAN_AND, "and"},
            {TokenType::BOOLEAN_OR, "or"},
            {TokenType::ENUM, "enum"},
            {TokenType::NIL, "nil"},
            {TokenType::IS, "is"},
            {TokenType::END, ""},
        };

        Lexer* l = lexer_create(input);
        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_destroy(l);
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

        Lexer* l = lexer_create(input);
        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_destroy(l);
    }

    SECTION("Dot operators") {
        const char* input = ". .. ..=";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::DOT, "."},
            {TokenType::DOT_DOT, ".."},
            {TokenType::DOT_DOT_EQ, "..="},
            {TokenType::END, ""},
        };

        Lexer* l = lexer_create(input);
        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_destroy(l);
    }

    SECTION("Control flow keywords") {
        const char* input = "if else match case return for while do continue break";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::IF, "if"},
            {TokenType::ELSE, "else"},
            {TokenType::MATCH, "match"},
            {TokenType::CASE, "case"},
            {TokenType::RETURN, "return"},
            {TokenType::FOR, "for"},
            {TokenType::WHILE, "while"},
            {TokenType::DO, "do"},
            {TokenType::CONTINUE, "continue"},
            {TokenType::BREAK, "break"},
            {TokenType::END, ""},
        };

        Lexer* l = lexer_create(input);
        for (const auto& [t, s] : expecteds) {
            Token token = lexer_next_token(l);
            REQUIRE(t == token.type);
            REQUIRE(slice_equals_str_z(&token.slice, s));
        }
        lexer_destroy(l);
    }
}
