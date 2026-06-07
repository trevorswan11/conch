#include <utility>

#include <catch2/catch_test_macros.hpp>

#include "helpers/ast.hh"
#include "syntax/error.hh"

namespace ghoti::tests {

TEST_CASE("Empty enum") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Enums must be declared with at least one enumeration",
                syntax::Error::EMPTY_ENUM,
                std::pair{0UZ, 0UZ}};
    };

    helpers::test_parser_fail("enum {};", expected_diag());
    helpers::test_parser_fail("enum : T {};", expected_diag());
}

TEST_CASE("Illegal underlying type") {
    helpers::test_parser_fail("enum : 4 {A};",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 7});
    helpers::test_parser_fail(R"(enum : "e" {A};)",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_IDENTIFIER, 0, 7});
}

TEST_CASE("Empty enum with decl") {
    helpers::test_parser_fail(
        "enum : i64 { const b := fn(&self, a: A): C { c; }; };",
        syntax::Diagnostic{"Enums must be declared with at least one enumeration",
                           syntax::Error::EMPTY_ENUM,
                           std::pair{0UZ, 0UZ}});
}

TEST_CASE("Out of order enum") {
    helpers::test_parser_fail("enum : i64 { A = 2l const b := fn(&self, a: A): C { c; }; B = 2l };",
                              syntax::Diagnostic{"Expected token SEMICOLON, found RBRACE",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 65UZ}});
}

TEST_CASE("Illegal struct members") {
    helpers::test_parser_fail(
        "struct { extern var foo: bar; };",
        syntax::Diagnostic{"Member declarations may neither be marked extern nor export",
                           syntax::Error::INVALID_MEMBER,
                           std::pair{0UZ, 9UZ}});

    helpers::test_parser_fail("struct { defer {}; };",
                              syntax::Diagnostic{"Expected token IDENT, found DEFER",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 9UZ}},
                              syntax::Diagnostic{"No prefix parse function for RBRACE(}) found",
                                                 syntax::Error::MISSING_PREFIX_PARSER,
                                                 std::pair{0UZ, 19UZ}});
}

TEST_CASE("Illegal union field name") {
    helpers::test_parser_fail(
        "union { 2: i32 };",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 8});
}

TEST_CASE("Illegal union field type") {
    helpers::test_parser_fail("union { a: 2 };",
                              syntax::Diagnostic{syntax::Error::ILLEGAL_EXPLICIT_TYPE, 0, 9});
}

TEST_CASE("Empty union") {
    helpers::test_parser_fail("union { };",
                              syntax::Diagnostic{"Unions must be declared with at least one field",
                                                 syntax::Error::EMPTY_UNION,
                                                 std::pair{0UZ, 0UZ}});
}

TEST_CASE("Empty union with decl") {
    helpers::test_parser_fail("union { const b := fn(&self, a: A): C { c; }; };",
                              syntax::Diagnostic{"Unions must be declared with at least one field",
                                                 syntax::Error::EMPTY_UNION,
                                                 std::pair{0UZ, 0UZ}});
}

TEST_CASE("Out of order union") {
    helpers::test_parser_fail("union { a: i32, const b := fn(&self, a: A): C { c; }; b: i32, };",
                              syntax::Diagnostic{"Expected token SEMICOLON, found COMMA",
                                                 syntax::Error::UNEXPECTED_TOKEN,
                                                 std::pair{0UZ, 60UZ}});
}

TEST_CASE("Illegal type aliasing/definition") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"User-defined types can only be defined with non-modified aliases",
                syntax::Error::ILLEGAL_USING_ALIAS_WITH_MODIFIERS,
                std::pair{0UZ, 10UZ}};
    };

    helpers::test_parser_fail("using U = &union { a: i32 };", expected_diag());
    helpers::test_parser_fail("using S = ^struct { pub var foo := bar; };", expected_diag());
    helpers::test_parser_fail("using E = ^enum { a };", expected_diag());
}

} // namespace ghoti::tests
