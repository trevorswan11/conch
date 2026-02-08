#include <catch_amalgamated.hpp>

#include "ast_nodes.hpp"

using namespace conch;

TEST_CASE("Normal identifiers") {
    helpers::test_ident("foobar");
    helpers::test_ident("int", TokenType::INT_TYPE);
    helpers::test_ident("uint", TokenType::UINT_TYPE);
    helpers::test_ident("float", TokenType::FLOAT_TYPE);
    helpers::test_ident("byte", TokenType::BYTE_TYPE);
    helpers::test_ident("string", TokenType::STRING_TYPE);
    helpers::test_ident("bool", TokenType::BOOL_TYPE);
    helpers::test_ident("void", TokenType::VOID_TYPE);
}
