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

TEST_CASE("Next Token") {
    SECTION("Symbols Only") {
        const char* input = "=+(){},;";

        std::vector<ExpectedToken> expecteds = {
            {TokenType::ASSIGN, "="},
            {TokenType::PLUS, "+"},
            {TokenType::LPAREN, "("},
            {TokenType::RPAREN, ")"},
            {TokenType::LBRACE, "{"},
            {TokenType::RBRACE, "}"},
            {TokenType::COMMA, ","},
            {TokenType::SEMICOLON, ";"},
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
