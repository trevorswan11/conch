#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("Non-terminated iterables") {
    helpers::test_parser_fail("for (0..4 |i| { a; } else return b;",
                              syntax::Diagnostic{"Expected token RBRACE, found IDENT",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 16UZ}},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 19UZ}});
}

TEST_CASE("Missing iterables") {
    helpers::test_parser_fail("for () |i| { a; } else return b;",
                              syntax::Diagnostic{"For loops must contain at least one iterable",
                                                 syntax::Error::FOR_MISSING_ITERABLES,
                                                 std::pair{0UZ, 0UZ}},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 16UZ}});

    helpers::test_parser_fail(
        "for |i| { a; } else return b;",
        syntax::Diagnostic{
            "Expected token LPAREN, found BW_OR", syntax::Error::UNEXPECTED_TOKEN, 0, 4},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 13UZ}});
}

TEST_CASE("Non-terminated captures") {
    helpers::test_parser_fail(
        "for (0..4) |i { a; } else return b;",
        syntax::Diagnostic{
            "Expected token COMMA, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 14},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 19UZ}});
}

TEST_CASE("Missing captures") {
    helpers::test_parser_fail(
        "for (0..4) { a; } else return b;",
        syntax::Diagnostic{
            "Expected token BW_OR, found LBRACE", syntax::Error::UNEXPECTED_TOKEN, 0, 11},
        syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 16UZ}});
}

TEST_CASE("Illegal capture") {
    helpers::test_parser_fail("for (0..4) |2| { a; } else return b;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 12},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 20UZ}});
}

TEST_CASE("Iterable-capture mismatch") {
    helpers::test_parser_fail(
        "for (0..4) |i, j| { a; } else return b;",
        syntax::Diagnostic{"The number of for loop captures must match the number of iterables",
                           syntax::Error::FOR_ITERABLE_CAPTURE_MISMATCH,
                           std::pair{0UZ, 0UZ}});
}

TEST_CASE("Empty for block") {
    helpers::test_parser_fail(
        "for (0..4) |i| {} else return b;",
        syntax::Diagnostic{"For loops' bodies must contain at least one statement",
                           syntax::Error::EMPTY_LOOP,
                           std::pair{0UZ, 15UZ}});
}

TEST_CASE("Illegal for-else clause") {
    helpers::test_parser_fail("for (0..4) |i| { a; } else import std;",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_LOOP_NON_BREAK, 0, 27});
}

} // namespace ghoti::tests
