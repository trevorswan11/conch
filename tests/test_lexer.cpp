#include "test_framework/catch_amalgamated.hpp"

#include <string.h>

#include <utility>
#include <vector>

extern "C" {
#include "lexer/lexer.h"
#include "token/token.h"
#include "util/mem.h"
}

TEST_CASE("Next Token") {
    const char* input = "=+(){},;";

    std::vector<std::pair<TokenType, const char*>> expecteds = {
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
        Token tok = lexer_next_token(l);

        REQUIRE(t == tok.type);
        REQUIRE(slice_equals_str(&tok.slice, s));
    }

    lexer_destroy(l);
}
