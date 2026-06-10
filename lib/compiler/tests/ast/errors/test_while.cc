#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("Empty do-while") {
    helpers::test_parser_fail(
        "do {} while (true);",
        syntax::Diagnostic{"Do-while loops' bodies must contain at least one statement",
                           syntax::Error::EMPTY_LOOP,
                           std::pair{0UZ, 3UZ}});
}

TEST_CASE("Missing do-while condition") {
    helpers::test_parser_fail("do {a; } while ();",
                              syntax::Diagnostic{"While loops must have a condition",
                                                 syntax::Error::WHILE_MISSING_CONDITION,
                                                 std::pair{0UZ, 15UZ}});
}

TEST_CASE("Unclosed do-while body") {
    helpers::test_parser_fail("do { while (true);",
                              syntax::Diagnostic{"Expected token LBRACE, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 17UZ}});
}

TEST_CASE("Unclosed do-while condition") {
    helpers::test_parser_fail("do {a; } while (true;",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 20UZ}});
}

TEST_CASE("Empty infinite loop") {
    helpers::test_parser_fail(
        "loop {};",
        syntax::Diagnostic{"Infinite loops' bodies must contain at least one statement",
                           syntax::Error::EMPTY_LOOP,
                           std::pair{0UZ, 5UZ}});
}

TEST_CASE("Empty while") {
    helpers::test_parser_fail(
        "while (true) {};",
        syntax::Diagnostic{
            "While loops without continuation expressions require a statement in their body",
            syntax::Error::EMPTY_LOOP,
            std::pair{0UZ, 13UZ}});
}

TEST_CASE("Missing while condition") {
    helpers::test_parser_fail("while () {a;};",
                              syntax::Diagnostic{"While loops must have a corresponding condition",
                                                 syntax::Error::WHILE_MISSING_CONDITION,
                                                 std::pair{0UZ, 0UZ}},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 12UZ}});
}

TEST_CASE("Unclosed while body") {
    helpers::test_parser_fail("while (true) {;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 14UZ}});
}

TEST_CASE("Unclosed while condition") {
    helpers::test_parser_fail("while (true {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 14UZ}});
}

TEST_CASE("Malformed while continuation") {
    helpers::test_parser_fail(
        "while (true) : () {};",
        syntax::Diagnostic{"Continuation expression was expected but not found",
                           syntax::Error::EMPTY_WHILE_CONTINUATION,
                           std::pair{0UZ, 11UZ}});

    helpers::test_parser_fail("while (true) : (i += 1 {};",
                              syntax::Diagnostic{"Expected token RPAREN, found SEMICOLON",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 25UZ}});
}

TEST_CASE("Illegal while-else clause") {
    helpers::test_parser_fail("while (true) : (i += 1) {} else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LOOP_NON_BREAK, 0, 32});

    helpers::test_parser_fail("while (true) : (i += 1) {} else;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 31UZ}});
}

} // namespace ghoti::tests
