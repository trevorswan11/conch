#include <array>
#include <initializer_list>
#include <sstream>
#include <string_view>
#include <utility>

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <stdx/types.hh>

#include "helpers/ast.hh"
#include "helpers/common.hh"
#include "syntax/error.hh"
#include "syntax/keywords.hh"

namespace ghoti::tests {

namespace keywords = syntax::keywords;

TEST_CASE("Non-terminated block") {
    helpers::test_parser_fail(
        "{ ",
        syntax::Diagnostic{
            "Expected token RBRACE, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 2});
}

namespace {

auto test_decl_fail(std::initializer_list<syntax::Keyword> modifiers,
                    syntax::Diagnostic&&                   expected_error,
                    std::string_view                       init = "a := 2;") -> void {
    std::stringstream ss;
    for (const auto& keyword : modifiers) { ss << keyword.name << " "; }
    ss << init;
    helpers::test_parser_fail(ss.view(), std::move(expected_error));
}

} // namespace

TEST_CASE("Mutability restrictions") {
    const auto expected_diag = [](usize mod_count = 2) -> syntax::Diagnostic {
        return {fmt::format("Exactly one mutability modifier may be used; found {}", mod_count),
                syntax::Error::ILLEGAL_DECL_MODIFIERS,
                std::pair{0UZ, 0UZ}};
    };

    constexpr std::array contending_mut{keywords::CONSTEXPR, keywords::VAR, keywords::CONSTANT};
    for (const auto& mut : helpers::combinations(contending_mut)) {
        test_decl_fail({mut.first, mut.second}, expected_diag());
    }
    test_decl_fail({keywords::CONSTEXPR, keywords::VAR, keywords::CONSTANT}, expected_diag(3));
}

TEST_CASE("Constexpr restrictions") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Extern values cannot be known at compile time",
                syntax::Error::ILLEGAL_DECL_MODIFIERS,
                std::pair{0UZ, 0UZ}};
    };

    test_decl_fail({keywords::CONSTEXPR, keywords::EXTERN}, expected_diag());
    test_decl_fail({keywords::EXTERN, keywords::CONSTEXPR}, expected_diag());
}

TEST_CASE("ABI/Linkage restrictions") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"At most one ABI-related modifier may be used; found 2",
                syntax::Error::ILLEGAL_DECL_MODIFIERS,
                std::pair{0UZ, 0UZ}};
    };

    test_decl_fail({keywords::EXPORT, keywords::EXTERN, keywords::VAR}, expected_diag());
    test_decl_fail({keywords::EXTERN, keywords::EXPORT, keywords::VAR}, expected_diag());
}

TEST_CASE("Extern requirements") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Extern declarations may not be value-initialized",
                syntax::Error::EXTERN_VALUE_INITIALIZED,
                std::pair{0UZ, 0UZ}};
    };

    test_decl_fail({keywords::EXTERN, keywords::CONSTANT}, expected_diag());
    test_decl_fail({keywords::EXTERN, keywords::VAR}, expected_diag());
}

TEST_CASE("Constant requirements") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Constant non-extern declarations must have an associated value",
                syntax::Error::CONST_DECL_MISSING_VALUE,
                std::pair{0UZ, 0UZ}};
    };

    test_decl_fail({keywords::CONSTANT}, expected_diag(), "a: i32;");
    test_decl_fail({keywords::CONSTEXPR}, expected_diag(), "a: i32;");
}

TEST_CASE("Non-terminated decls") {
    helpers::test_parser_fail(
        "var a: i32 = 2",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Duplicate declaration modifier") {
    helpers::test_parser_fail(
        "var var a: i32;",
        syntax::Diagnostic{"Declaration modifiers may only be used once in any order",
                           syntax::Error::DUPLICATE_DECL_MODIFIER,
                           std::pair{0UZ, 4UZ}});
}

TEST_CASE("Illegal deferred statements") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Deferred statements must be expressions, discards, or blocks",
                syntax::Error::ILLEGAL_DEFERRED_STATEMENT,
                std::pair{0UZ, 6UZ}};
    };

    helpers::test_parser_fail("defer import std;", expected_diag());
    helpers::test_parser_fail("defer return 3;", expected_diag());
    helpers::test_parser_fail("defer var a: i32;", expected_diag());
    helpers::test_parser_fail("defer using a = i32;", expected_diag());
}

TEST_CASE("Missing deferred statements") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Defer statements require an statement to defer",
                syntax::Error::DEFER_MISSING_DEFERREE,
                std::pair{0UZ, 0UZ}};
    };

    helpers::test_parser_fail("defer", expected_diag());
    helpers::test_parser_fail("defer;", expected_diag());
}

TEST_CASE("Misplaced correct discardedstatement") {
    helpers::test_parser_fail(
        "_ = import std;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 4UZ}});
}

TEST_CASE("Misplaced incorrect discarded statement") {
    helpers::test_parser_fail(
        "_ = import 3;",
        syntax::Diagnostic{"No prefix parse function for IMPORT(import) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 4UZ}});
}

TEST_CASE("Missing discardee") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Discarded statements must have a statement to discard",
                syntax::Error::DISCARD_MISSING_DISCARDEE,
                std::pair{0UZ, 2UZ}};
    };

    helpers::test_parser_fail("_ = ", expected_diag());
    helpers::test_parser_fail("_ = ;", expected_diag());
}

TEST_CASE("Incorrect library imports") {
    const auto expected_diag = [] -> syntax::Diagnostic {
        return {"Imported payloads may only be filename strings or module identifiers",
                syntax::Error::ILLEGAL_IMPORT_TYPE,
                std::pair{0UZ, 7UZ}};
    };

    helpers::test_parser_fail("import 2;", expected_diag());
    helpers::test_parser_fail("import as 2;", expected_diag());
    helpers::test_parser_fail("import 2 as 3;", expected_diag());

    helpers::test_parser_fail(
        "import std as 2;",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 14});
}

TEST_CASE("Incorrect file imports ") {
    helpers::test_parser_fail(R"(import "";)",
                              syntax::Diagnostic{"File import names cannot be empty",
                                                 syntax::Error::EMPTY_FILE_IMPORT,
                                                 std::pair{0UZ, 7UZ}});

    helpers::test_parser_fail(R"(import "" as e;)",
                              syntax::Diagnostic{"File import names cannot be empty",
                                                 syntax::Error::EMPTY_FILE_IMPORT,
                                                 std::pair{0UZ, 7UZ}});

    helpers::test_parser_fail(
        R"(import "ast/node.p";)",
        syntax::Diagnostic{"All file imports must be aliased to an identifier",
                           syntax::Error::FILE_IMPORT_MISSING_ALIAS,
                           std::pair{0UZ, 0UZ}});

    helpers::test_parser_fail(
        R"(import "ast/node.p" as 2;)",
        syntax::Diagnostic{
            "Expected token IDENT, found INT_10", syntax::Error::UNEXPECTED_TOKEN, 0, 23});
}

TEST_CASE("Non-terminated imports") {
    helpers::test_parser_fail(
        "import std",
        syntax::Diagnostic{
            "Expected token SEMICOLON, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 10});
}

TEST_CASE("Incorrectly terminated jumps") {
    using namespace std::string_view_literals;
    constexpr std::array inputs{"return"sv, "continue"sv, "break"sv};
    for (const auto& input : inputs) {
        helpers::test_parser_fail(input,
                                  syntax::Diagnostic{"Expected token SEMICOLON, found END",
                                                     syntax::Error::UNEXPECTED_TOKEN,
                                                     std::pair{0UZ, input.size()}});
    }

    helpers::test_parser_fail(
        "return return",
        syntax::Diagnostic{"No prefix parse function for RETURN(return) found",
                           syntax::Error::MISSING_PREFIX_PARSER,
                           std::pair{0UZ, 7UZ}});
}

TEST_CASE("Illegal continue/break control flow") {
    helpers::test_parser_fail("continue 4;",
                              syntax::Diagnostic{"Continue statements may only contain labels",
                                                 syntax::Error::VALUED_CONTINUE,
                                                 std::pair{0UZ, 0UZ}});

    helpers::test_parser_fail("break 4;",
                              syntax::Diagnostic{"Valued break statements must be labeled",
                                                 syntax::Error::VALUED_BREAK_MISSING_LABEL,
                                                 std::pair{0UZ, 0UZ}});
}

TEST_CASE("Non-terminated test") {
    helpers::test_parser_fail(
        "test {",
        syntax::Diagnostic{
            "Expected token RBRACE, found END", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Empty test description") {
    helpers::test_parser_fail(R"(test "" {};)",
                              syntax::Diagnostic{"Test descriptions may not be empty when present",
                                                 syntax::Error::EMPTY_TEST_DESCRIPTION,
                                                 std::pair{0UZ, 5UZ}});
}

TEST_CASE("Missing alias") {
    helpers::test_parser_fail(
        "using &[0x2UZ][N]*E;",
        syntax::Diagnostic{
            "Expected token IDENT, found BW_AND", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

TEST_CASE("Missing type") {
    helpers::test_parser_fail(
        "using T;",
        syntax::Diagnostic{
            "Expected token ASSIGN, found SEMICOLON", syntax::Error::UNEXPECTED_TOKEN, 0, 7});
}

TEST_CASE("Illegal identifier alias") {
    helpers::test_parser_fail(
        "using type = T;",
        syntax::Diagnostic{
            "Expected token IDENT, found TYPE_TYPE", syntax::Error::UNEXPECTED_TOKEN, 0, 6});
}

} // namespace ghoti::tests
