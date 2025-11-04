#include "test_framework/catch_amalgamated.hpp"

#include <string.h>

extern "C" {
#include "token/token.h"
}

TEST_CASE("TokenType conversion") {
    REQUIRE(strcmp(string_from_token_type(TokenType::END), "EOF") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::IDENT), "IDENT") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::INT), "INT") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::ASSIGN), "=") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::PLUS), "+") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::COMMA), ",") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::SEMICOLON), ";") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::LPAREN), "(") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::RPAREN), ")") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::LBRACE), "{") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::RBRACE), "}") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::FUNCTION), "FUNCTION") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::LET), "LET") == 0);
    REQUIRE(strcmp(string_from_token_type(TokenType::ILLEGAL), "ILLEGAL") == 0);

    REQUIRE(strcmp(string_from_token_type((TokenType)-1), "ILLEGAL") == 0);
    REQUIRE(strcmp(string_from_token_type((TokenType)100), "ILLEGAL") == 0);
}
