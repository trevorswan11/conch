#include "catch_amalgamated.hpp"

#include <stdio.h>
#include <string.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/float.h"
#include "ast/expressions/integer.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/statement.h"

#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/math.h"
#include "util/status.h"
}

FileIO stdio = file_io_std();

struct ParserFixture {
    ParserFixture(const char* input) {
        REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
        REQUIRE(STATUS_OK(lexer_consume(&l)));

        REQUIRE(STATUS_OK(ast_init(&ast, standard_allocator)));

        REQUIRE(STATUS_OK(parser_init(&p, &l, &stdio, standard_allocator)));
        REQUIRE(STATUS_OK(parser_consume(&p, &ast)));
    }

    ~ParserFixture() {
        parser_deinit(&p);
        ast_deinit(&ast);
        lexer_deinit(&l);
    }

    Parser p;
    AST    ast;
    Lexer  l;
};

static inline void check_parse_errors(Parser*                  p,
                                      std::vector<std::string> expected_errors,
                                      bool                     print_anyways = false) {
    const ArrayList* actual_errors = &p->errors;
    MutSlice         error;

    if (actual_errors->length == 0 && expected_errors.size() == 0) {
        return;
    } else if (print_anyways && actual_errors->length != 0) {
        for (size_t i = 0; i < actual_errors->length; i++) {
            REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
            std::cerr << "Parser error: " << error.ptr << "\n";
        }
    }

    REQUIRE(actual_errors->length == expected_errors.size());

    for (size_t i = 0; i < actual_errors->length; i++) {
        REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
        std::string expected = expected_errors[i];
        REQUIRE(mut_slice_equals_str_z(&error, expected.c_str()));
    }
}

static inline void
test_decl_statement(Statement* stmt, bool expect_const, const char* expected_ident) {
    Node* stmt_node = (Node*)stmt;
    Slice literal   = stmt_node->vtable->token_literal(stmt_node);
    REQUIRE(slice_equals_str_z(&literal, expect_const ? "const" : "var"));

    DeclStatement*        decl_stmt = (DeclStatement*)stmt;
    IdentifierExpression* ident     = decl_stmt->ident;
    Node*                 node      = (Node*)ident;
    literal                         = node->vtable->token_literal(node);
    REQUIRE(slice_equals_str_z(&literal, token_type_name(TokenType::IDENT)));

    REQUIRE(mut_slice_equals_str_z(&decl_stmt->ident->name, expected_ident));
}

TEST_CASE("Declarations") {
    SECTION("Var statements") {
        const char*   input = "var x := 5;\n"
                              "var y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        REQUIRE(pf.ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            test_decl_statement(stmt, false, expected_identifiers[i]);
        }
    }

    SECTION("Var statements with errors") {
        const char*   input = "var x 5;\n"
                              "var = 10;\n"
                              "var 838383;\n"
                              "var z := 6";
        ParserFixture pf(input);

        std::vector<std::string> expected_errors = {
            "Expected token WALRUS, found INT_10 [Ln 1, Col 7]",
            "Expected token IDENT, found ASSIGN [Ln 2, Col 5]",
            "Expected token IDENT, found INT_10 [Ln 3, Col 5]",
        };

        check_parse_errors(&pf.p, expected_errors);
        REQUIRE(pf.ast.statements.length == 0);
    }

    SECTION("Var and const statements") {
        const char*   input = "var x := 5;\n"
                              "const y := 10;\n"
                              "var foobar := 838383;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);

        std::vector<const char*> expected_identifiers = {"x", "y", "foobar"};
        std::vector<bool>        is_const             = {false, true, false};
        REQUIRE(pf.ast.statements.length == expected_identifiers.size());

        Statement* stmt;
        for (size_t i = 0; i < expected_identifiers.size(); i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            test_decl_statement(stmt, is_const[i], expected_identifiers[i]);
        }
    }
}

TEST_CASE("Return statements") {
    SECTION("Happy returns") {
        const char*   input = "return 5;\n"
                              "return 10;\n"
                              "return 993322;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);
        REQUIRE(pf.ast.statements.length == 3);

        Statement* stmt;
        for (size_t i = 0; i < pf.ast.statements.length; i++) {
            REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, i, &stmt)));
            Slice literal = stmt->base.vtable->token_literal((Node*)stmt);
            REQUIRE(slice_equals_str_z(&literal, "return"));
        }
    }

    SECTION("Returns w/o sentinel semicolon") {
        const char*   input = "return 5";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {}, true);
    }
}

TEST_CASE("Identifier Expressions") {
    SECTION("Single arbitrary identifier") {
        const char*   input = "foobar;";
        ParserFixture pf(input);

        check_parse_errors(&pf.p, {}, true);
        REQUIRE(pf.ast.statements.length == 1);

        Statement* stmt;
        REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));

        Node* node    = (Node*)stmt;
        Slice literal = node->vtable->token_literal(node);
        REQUIRE(slice_equals_str_z(&literal, "foobar"));

        ExpressionStatement*  expr_stmt = (ExpressionStatement*)stmt;
        IdentifierExpression* ident     = (IdentifierExpression*)expr_stmt->expression;
        REQUIRE(ident->token_type == TokenType::IDENT);
        REQUIRE(mut_slice_equals_str_z(&ident->name, "foobar"));
    }
}

template <typename T>
static inline void
test_number_expression(const char* input, const char* expected_literal, T expected_value) {
    ParserFixture pf(input);

    check_parse_errors(&pf.p, {}, true);
    REQUIRE(pf.ast.statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&pf.ast.statements, 0, &stmt)));

    ExpressionStatement* expr = (ExpressionStatement*)stmt;

    if constexpr (std::is_same_v<T, double>) {
        FloatLiteralExpression* f = (FloatLiteralExpression*)expr->expression;
        REQUIRE(f->value == expected_value);
        Node* f_node  = (Node*)f;
        Slice literal = f_node->vtable->token_literal(f_node);
        REQUIRE(slice_equals_str_z(&literal, expected_literal));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        IntegerLiteralExpression* i = (IntegerLiteralExpression*)expr->expression;
        REQUIRE(i->value == expected_value);
        Node* i_node  = (Node*)i;
        Slice literal = i_node->vtable->token_literal(i_node);
        REQUIRE(slice_equals_str_z(&literal, expected_literal));
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        UnsignedIntegerLiteralExpression* i = (UnsignedIntegerLiteralExpression*)expr->expression;
        REQUIRE(i->value == expected_value);
        Node* i_node  = (Node*)i;
        Slice literal = i_node->vtable->token_literal(i_node);
        REQUIRE(slice_equals_str_z(&literal, expected_literal));
    } else {
        REQUIRE(false);
    }
}

TEST_CASE("Number-based expressions") {
    SECTION("Signed integer bases") {
        test_number_expression<int64_t>("5;", "5", 5);
        test_number_expression<int64_t>("0b10011101101;", "0b10011101101", 0b10011101101);
        test_number_expression<int64_t>("0o1234567;", "0o1234567", 342391);
        test_number_expression<int64_t>("0xFF8a91d;", "0xFF8a91d", 0xFF8a91d);
    }

    SECTION("Signed integer overflow") {
        const char*   input = "0xFFFFFFFFFFFFFFFF";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {"SIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
    }

    SECTION("Unsigned integer bases") {
        test_number_expression<uint64_t>("5u;", "5u", 5);
        test_number_expression<uint64_t>("0b10011101101u;", "0b10011101101u", 0b10011101101);
        test_number_expression<uint64_t>("0o1234567U;", "0o1234567U", 342391);
        test_number_expression<uint64_t>("0xFF8a91du;", "0xFF8a91du", 0xFF8a91d);

        test_number_expression<uint64_t>(
            "0xFFFFFFFFFFFFFFFFu;", "0xFFFFFFFFFFFFFFFFu", 0xFFFFFFFFFFFFFFFF);
    }

    SECTION("Unsigned integer overflow") {
        const char*   input = "0x10000000000000000u";
        ParserFixture pf(input);
        check_parse_errors(&pf.p, {"UNSIGNED_INTEGER_OVERFLOW [Ln 1, Col 1]"});
    }

    SECTION("Floating points") {
        test_number_expression<double>("1023.0;", "1023.0", 1023.0);
        test_number_expression<double>("1023.234612;", "1023.234612", 1023.234612);
        test_number_expression<double>("1023.234612e234;", "1023.234612e234", 1023.234612e234);
    }
}
