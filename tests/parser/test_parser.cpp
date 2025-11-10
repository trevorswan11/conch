#include "catch_amalgamated.hpp"

#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "util/containers/array_list.h"
}

static void check_parse_errors(Parser*                  p,
                               std::vector<std::string> expected_errors,
                               bool                     print_anyways = false) {
    const ArrayList* actual_errors = &p->errors;
    MutSlice         error;

    if (actual_errors->length == 0 && expected_errors.size() == 0) {
        return;
    } else if (print_anyways && actual_errors->length != 0) {
        for (size_t i = 0; i < actual_errors->length; i++) {
            REQUIRE(array_list_get(actual_errors, i, &error));
            std::cerr << "Parser error: " << error.ptr << "\n";
        }
    }

    REQUIRE(actual_errors->length == expected_errors.size());

    for (size_t i = 0; i < actual_errors->length; i++) {
        REQUIRE(array_list_get(actual_errors, i, &error));
        std::string expected = expected_errors[i];
        REQUIRE(mut_slice_equals_str_z(&error, expected.c_str()));
    }
}

static void test_decl_statement(Statement* stmt, bool expect_const, const char* expected_ident) {
    const char* literal = stmt->base.vtable->token_literal((Node*)stmt);
    REQUIRE(strcmp(literal,
                   expect_const ? token_type_name(TokenType::CONST)
                                : token_type_name(TokenType::VAR)) == 0);

    DeclStatement*        decl_stmt = (DeclStatement*)stmt;
    IdentifierExpression* ident     = decl_stmt->ident;
    Node*                 node      = (Node*)ident;
    literal                         = node->vtable->token_literal(node);
    REQUIRE(strcmp(literal, token_type_name(TokenType::IDENT)) == 0);

    REQUIRE(mut_slice_equals_str_z(&decl_stmt->ident->name, expected_ident));
}

TEST_CASE("Declarations") {
    FileIO stdio;
    file_io_init(&stdio, stdin, stdout, stderr);

    SECTION("Var statements") {
        const char* input = "var x := 5;\n"
                            "var y := 10;\n"
                            "var foobar := 838383;";
        Lexer       l;
        REQUIRE(lexer_init(&l, input));
        REQUIRE(lexer_consume(&l));

        AST ast;
        REQUIRE(ast_init(&ast));

        Parser p;
        REQUIRE(parser_init(&p, &l, &stdio));
        REQUIRE(parser_consume(&p, &ast));

        std::vector<std::string> expected_errors = {};
        check_parse_errors(&p, expected_errors, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        REQUIRE(ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(array_list_get(&ast.statements, i, &stmt));
            test_decl_statement(stmt, false, expected_identifiers[i]);
        }

        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }

    SECTION("Var statements with errors") {
        const char* input = "var x 5;\n"
                            "var = 10;\n"
                            "var 838383;\n"
                            "var z := 6";
        Lexer       l;
        REQUIRE(lexer_init(&l, input));
        REQUIRE(lexer_consume(&l));

        AST ast;
        REQUIRE(ast_init(&ast));

        Parser p;
        REQUIRE(parser_init(&p, &l, &stdio));
        REQUIRE(parser_consume(&p, &ast));

        std::vector<std::string> expected_errors = {
            "Expected token WALRUS, found INT_10 [Ln 1, Col 7]",
            "Expected token IDENT, found ASSIGN [Ln 2, Col 5]",
            "Expected token IDENT, found INT_10 [Ln 3, Col 5]",
        };

        check_parse_errors(&p, expected_errors);
        REQUIRE(ast.statements.length == 0);

        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }

    SECTION("Var and const statements") {
        const char* input = "var x := 5;\n"
                            "const y := 10;\n"
                            "var foobar := 838383;";
        Lexer       l;
        REQUIRE(lexer_init(&l, input));
        REQUIRE(lexer_consume(&l));

        AST ast;
        REQUIRE(ast_init(&ast));

        Parser p;
        REQUIRE(parser_init(&p, &l, &stdio));
        REQUIRE(parser_consume(&p, &ast));

        std::vector<std::string> expected_errors = {};
        check_parse_errors(&p, expected_errors, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        std::vector<bool>        is_const             = {false, true, false};
        REQUIRE(ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(array_list_get(&ast.statements, i, &stmt));
            test_decl_statement(stmt, is_const[i], expected_identifiers[i]);
        }

        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }
}

TEST_CASE("Return statements") {
    FileIO stdio;
    file_io_init(&stdio, stdin, stdout, stderr);

    SECTION("Happy returns") {
        const char* input = "return 5;\n"
                            "return 10;\n"
                            "return 993322;";
        Lexer       l;
        REQUIRE(lexer_init(&l, input));
        REQUIRE(lexer_consume(&l));

        AST ast;
        REQUIRE(ast_init(&ast));

        Parser p;
        REQUIRE(parser_init(&p, &l, &stdio));
        REQUIRE(parser_consume(&p, &ast));

        std::vector<std::string> expected_errors = {};
        check_parse_errors(&p, expected_errors, true);

        REQUIRE(ast.statements.length == 3);

        Statement* stmt;
        for (size_t i = 0; i < ast.statements.length; i++) {
            REQUIRE(array_list_get(&ast.statements, i, &stmt));
            const char* literal = stmt->base.vtable->token_literal((Node*)stmt);
            REQUIRE(strcmp(literal, token_type_name(TokenType::RETURN)) == 0);
        }

        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }

    SECTION("Returns w/o sentinel semicolon") {
        const char* input = "return 5";
        Lexer       l;
        REQUIRE(lexer_init(&l, input));
        REQUIRE(lexer_consume(&l));

        AST ast;
        REQUIRE(ast_init(&ast));

        Parser p;
        REQUIRE(parser_init(&p, &l, &stdio));
        REQUIRE(parser_consume(&p, &ast));

        std::vector<std::string> expected_errors = {};
        check_parse_errors(&p, expected_errors, true);

        REQUIRE(ast.statements.length == 0);

        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }
}
