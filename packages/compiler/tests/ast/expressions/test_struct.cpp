#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>

#include "ast/helpers.hpp"

#include "ast/expressions/function.hpp"
#include "ast/expressions/struct.hpp"
#include "ast/expressions/type.hpp"
#include "ast/statements/declaration.hpp"

#include "lexer/operators.hpp"

namespace conch::tests {

namespace helpers {

template <typename... Ds>
    requires(std::same_as<Ds, ast::DeclStatement> && ...)
auto make_decls(Ds&&... decls) -> std::vector<Box<ast::DeclStatement>> {
    return make_vector<Box<ast::DeclStatement>>(make_box<Ds>(std::forward<Ds>(decls))...);
}

} // namespace helpers

namespace mods = helpers::type_modifiers;

TEST_CASE("Struct flavors") {
    const std::string_view input{
        "{ var a: Foo = bar; const b := fn(*mut this, a: A, b: *B): C { c; }; };"};
    const auto cases = std::to_array<std::pair<std::string, Keyword>>({
        {fmt::format("struct {}", input), keywords::STRUCT},
        {fmt::format("packed struct {}", input), keywords::PACKED},
    });

    for (const auto& input_case : cases) {
        helpers::test_expr_stmt(
            input_case.first,
            ast::StructExpression{
                Token{input_case.second},
                helpers::make_decls(
                    ast::DeclStatement{
                        Token{keywords::VAR},
                        helpers::make_ident("a"),
                        make_box<ast::TypeExpression>(Token{TokenType::COLON, ":"},
                                                      ast::ExplicitType{
                                                          mods::BASE,
                                                          helpers::make_ident("Foo"),
                                                      }),
                        helpers::make_ident("bar"),
                        ast::DeclModifiers::VARIABLE,
                    },
                    ast::DeclStatement{
                        Token{keywords::CONST},
                        helpers::make_ident("b"),
                        make_box<ast::TypeExpression>(Token{operators::WALRUS}, nullopt),
                        make_box<ast::FunctionExpression>(
                            Token{keywords::FN},
                            ast::SelfParameter{mods::MUT_PTR, helpers::make_ident("this")},
                            helpers::make_parameters(
                                ast::FunctionParameter{
                                    helpers::make_ident("a"),
                                    ast::ExplicitType{mods::BASE, helpers::make_ident("A")}},
                                ast::FunctionParameter{
                                    helpers::make_ident("b"),
                                    ast::ExplicitType{mods::PTR, helpers::make_ident("B")}}),
                            ast::ExplicitType{mods::BASE, helpers::make_ident("C")},
                            helpers::make_expr_block_stmt(helpers::ident_from("c"))),
                        ast::DeclModifiers::CONSTANT,
                    })});
    }
}

TEST_CASE("Illegal struct member") {
    helpers::test_fail("struct { import std; };",
                       ParserDiagnostic{ParserError::INVALID_STRUCT_MEMBER, 1, 10});
}

TEST_CASE("Empty struct body") {
    helpers::test_fail("struct {};", ParserDiagnostic{ParserError::EMPTY_STRUCT, 1, 1});
}

TEST_CASE("Packed keyword out of order") {
    helpers::test_fail("struct packed { var a: Foo = bar; };",
                       ParserDiagnostic{ParserError::PACKED_AFTER_STRUCT_KEYWORD, 1, 1},
                       ParserDiagnostic{"No prefix parse function for RBRACE(}) found",
                                        ParserError::MISSING_PREFIX_PARSER,
                                        1,
                                        35});
}

} // namespace conch::tests
