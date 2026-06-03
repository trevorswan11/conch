#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

#include "types.hh"

namespace porpoise::tests {

TEST_CASE("Array size token requirement") {
    helpers::test_parser_fail(
        "[]i32{2};",
        syntax::Diagnostic{"Array literals must be initialized with an implicit or explicit size",
                           syntax::Error::MISSING_ARRAY_SIZE_TOKEN,
                           std::pair{0uz, 0uz}});
}

TEST_CASE("No arguments with comma") {
    helpers::test_parser_fail("func(,)",
                              syntax::Diagnostic{"A comma implies an argument but none were found",
                                                 syntax::Error::COMMA_WITH_MISSING_CALL_ARGUMENT,
                                                 std::pair{0uz, 5uz}});
}

TEST_CASE("Non-comma separated arguments") {
    helpers::test_parser_fail(
        "func(1 2)",
        syntax::Diagnostic{
            "Expected token COMMA, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Non-terminated identifier") {
    helpers::test_parser_fail(
        "foobar",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("No index") {
    helpers::test_parser_fail(
        "arr[]",
        syntax::Diagnostic{"Cannot index into an array without an index expression",
                           syntax::Error::INDEX_MISSING_EXPRESSION,
                           std::pair{0uz, 3uz}});
}

TEST_CASE("Illegal infix node") {
    helpers::test_parser_fail(
        "a and import std;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0uz, 6uz}});
}

TEST_CASE("Non-terminated infix") {
    helpers::test_parser_fail("a and;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 5uz}});

    helpers::test_parser_fail("a and",
                              syntax::Diagnostic{"Infix expressions require a right-hand operand",
                                                 syntax::Error::INFIX_MISSING_RHS,
                                                 std::pair{0uz, 2uz}});
}

TEST_CASE("Unclosed implicit initializer") {
    helpers::test_parser_fail(".{",
                              syntax::Diagnostic{"Expected token RBRACE, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 2uz}});

    helpers::test_parser_fail(".{ .a = 2",
                              syntax::Diagnostic{"Expected token COMMA, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 9uz}});
}

TEST_CASE("Unclosed explicit initializer") {
    helpers::test_parser_fail("T{",
                              syntax::Diagnostic{"Expected token RBRACE, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 2uz}});

    helpers::test_parser_fail("T{ .a = 2",
                              syntax::Diagnostic{"Expected token COMMA, found END",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0uz, 9uz}});
}

TEST_CASE("Malformed initializer key-value") {
    helpers::test_parser_fail("T{ .a = };",
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 8uz}});
}

TEST_CASE("Non-ident label") {
    helpers::test_parser_fail("2: {};",
                              syntax::Diagnostic{"Labels may only be identifiers",
                                                 syntax::Error::ILLEGAL_LABEL,
                                                 std::pair{0uz, 1uz}});
}

TEST_CASE("Illegal label expressions") {
    const auto expected_diag = [](usize ln = 0uz, usize col = 3uz) {
        return syntax::Diagnostic{"Labeled expressions may only be conditionals or loops",
                                  syntax::Error::ILLEGAL_LABEL_EXPRESSION,
                                  std::pair{ln, col}};
    };

    helpers::test_parser_fail("a: 3;", expected_diag());
    helpers::test_parser_fail("a: b;", expected_diag());
    helpers::test_parser_fail("a: b();", expected_diag());
    helpers::test_parser_fail("a: b: c: {};", expected_diag(0, 6));
}

TEST_CASE("Illegal label statements") {
    const auto expected_diag = [] {
        return syntax::Diagnostic{"Labeled statements may only be blocks",
                                  syntax::Error::ILLEGAL_LABEL_STATEMENT,
                                  std::pair{0uz, 3uz}};
    };

    helpers::test_parser_fail("a: defer 3;", expected_diag());
    helpers::test_parser_fail("a: return 3;", expected_diag());
}

TEST_CASE("Illegal implicit access operand") {
    helpers::test_parser_fail(".a::b",
                              syntax::Diagnostic{"Implicitly accessed names must be identifiers",
                                                 syntax::Error::ILLEGAL_IMPLICIT_ACCESS_OPERAND,
                                                 std::pair{0uz, 2uz}});
}

TEST_CASE("Prefix without operand") {
    helpers::test_parser_fail(".",
                              syntax::Diagnostic{"Prefix expressions require an operand",
                                                 syntax::Error::PREFIX_MISSING_OPERAND,
                                                 std::pair{0uz, 0uz}});

    helpers::test_parser_fail("!;",
                              syntax::Diagnostic{"No prefix parse function for SEMICOLON(;) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0uz, 1uz}});
}

TEST_CASE("Missing inner scope of resolution expression") {
    helpers::test_parser_fail(
        "A:: ;",
        syntax::Diagnostic{
            "Expected token IDENT, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 4});
}

TEST_CASE("Illegal inner scope of resolution expression") {
    helpers::test_parser_fail(
        "A::2;",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 3});
}

TEST_CASE("Illegal outer scope of resolution expression") {
    helpers::test_parser_fail(
        "2::A;",
        syntax::Diagnostic{"Module access expressions must have outer accessors or identifiers",
                           syntax::Error::ILLEGAL_OUTER_ACCESSOR_TYPE,
                           std::pair{0uz, 0uz}});
}

TEST_CASE("Missing inner member of dot expression") {
    helpers::test_parser_fail(
        "A. ;",
        syntax::Diagnostic{
            "Expected token IDENT, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 3});
}

TEST_CASE("Illegal inner member of dot expression") {
    helpers::test_parser_fail(
        "A.2;",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 2});
}

TEST_CASE("Illegal outer object of dot expression") {
    helpers::test_parser_fail(
        "fn():i32{}.a;",
        syntax::Diagnostic{"Dot expressions must have outer accessors or identifiers",
                           syntax::Error::ILLEGAL_OUTER_ACCESSOR_TYPE,
                           std::pair{0uz, 0uz}});
}

} // namespace porpoise::tests
