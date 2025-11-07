#include "catch_amalgamated.hpp"

#include <string.h>

#include <utility>
#include <vector>

extern "C" {
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "util/mem.h"
}

using ExpectedToken = std::pair<TokenType, const char*>;

TEST_CASE("Basic next token") {
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
            {TokenType::ASTERISK, "*"},
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
        const char* input = "let five = 5;\n"
                            "let ten = 10;\n\n"
                            "let add = fn(x, y) {\n"
                            "   x + y;\n"
                            "};\n\n"
                            "let result = add(five, ten);\n"
                            "let four_and_some = 4.2;";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::LET, "let"},     {TokenType::IDENT, "five"},
            {TokenType::ASSIGN, "="},    {TokenType::INT, "5"},
            {TokenType::SEMICOLON, ";"}, {TokenType::LET, "let"},
            {TokenType::IDENT, "ten"},   {TokenType::ASSIGN, "="},
            {TokenType::INT, "10"},      {TokenType::SEMICOLON, ";"},
            {TokenType::LET, "let"},     {TokenType::IDENT, "add"},
            {TokenType::ASSIGN, "="},    {TokenType::FUNCTION, "fn"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "x"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "y"},
            {TokenType::RPAREN, ")"},    {TokenType::LBRACE, "{"},
            {TokenType::IDENT, "x"},     {TokenType::PLUS, "+"},
            {TokenType::IDENT, "y"},     {TokenType::SEMICOLON, ";"},
            {TokenType::RBRACE, "}"},    {TokenType::SEMICOLON, ";"},
            {TokenType::LET, "let"},     {TokenType::IDENT, "result"},
            {TokenType::ASSIGN, "="},    {TokenType::IDENT, "add"},
            {TokenType::LPAREN, "("},    {TokenType::IDENT, "five"},
            {TokenType::COMMA, ","},     {TokenType::IDENT, "ten"},
            {TokenType::RPAREN, ")"},    {TokenType::SEMICOLON, ";"},
            {TokenType::LET, "let"},     {TokenType::IDENT, "four_and_some"},
            {TokenType::ASSIGN, "="},    {TokenType::FLOAT, "4.2"},
            {TokenType::SEMICOLON, ";"}, {TokenType::END, ""},
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

TEST_CASE("Numbers") {
    SECTION("Correct ints and floats") {
        const char* input = "0 123 3.14 42.0";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::INT, "0"},
            {TokenType::INT, "123"},
            {TokenType::FLOAT, "3.14"},
            {TokenType::FLOAT, "42.0"},
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

    SECTION("Illegal Floats") {
        SKIP(); // TODO: FIXME
        const char* input = ".0 1..2 3.4.5";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::DOT, "."},
            {TokenType::INT, "0"},
            {TokenType::INT, "1"},
            {TokenType::DOT_DOT, ".."},
            {TokenType::INT, "2"},
            {TokenType::FLOAT, "3.4"},
            {TokenType::DOT, "."},
            {TokenType::INT, "5"},
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

TEST_CASE("Advanced next token") {
    SECTION("Keywords") {
        const char* input = "struct true false and or";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::STRUCT, "struct"},
            {TokenType::TRUE, "true"},
            {TokenType::FALSE, "false"},
            {TokenType::BOOLEAN_AND, "and"},
            {TokenType::BOOLEAN_OR, "or"},
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

    SECTION("Compound operators") {
        const char* input = "+ += - -= * *= / /= & &= | |= << <<= >> >>= ~ ~=";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::PLUS, "+"},     {TokenType::PLUS_ASSIGN, "+="},
            {TokenType::MINUS, "-"},    {TokenType::MINUS_ASSIGN, "-="},
            {TokenType::ASTERISK, "*"}, {TokenType::ASTERISK_ASSIGN, "*="},
            {TokenType::SLASH, "/"},    {TokenType::SLASH_ASSIGN, "/="},
            {TokenType::AND, "&"},      {TokenType::AND_ASSIGN, "&="},
            {TokenType::OR, "|"},       {TokenType::OR_ASSIGN, "|="},
            {TokenType::SHL, "<<"},     {TokenType::SHL_ASSIGN, "<<="},
            {TokenType::SHR, ">>"},     {TokenType::SHR_ASSIGN, ">>="},
            {TokenType::NOT, "~"},      {TokenType::NOT_ASSIGN, "~="},
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
}
