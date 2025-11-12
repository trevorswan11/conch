#include "catch_amalgamated.hpp"

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/node.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"
#include "lexer/keywords.h"
#include "lexer/token.h"
#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"
}

TEST_CASE("AST Manual Reconstruction") {
    AST ast;
    REQUIRE(STATUS_OK(ast_init(&ast)));

    IdentifierExpression* ident_lhs;
    REQUIRE(STATUS_OK(identifier_expression_create(slice_from_str_z("my_var"), &ident_lhs)));

    IdentifierExpression* ident_rhs;
    REQUIRE(STATUS_OK(identifier_expression_create(slice_from_str_z("another_var"), &ident_rhs)));

    DeclStatement* decl;
    REQUIRE(STATUS_OK(decl_statement_create(
        token_init(TokenType::VAR, KEYWORD_VAR.slice.ptr, KEYWORD_VAR.slice.length, 0, 0),
        ident_lhs,
        (Expression*)ident_rhs,
        &decl)));

    Statement* stmt = (Statement*)decl;
    array_list_push_assume_capacity(&ast.statements, &stmt);

    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 30)));
    REQUIRE(STATUS_OK(ast_reconstruct(&ast, &sb)));

    MutSlice reconstructed;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &reconstructed)));
    REQUIRE(reconstructed.ptr);
    REQUIRE(mut_slice_equals_str_z(&reconstructed, "var my_var = another_var;"));
    free(reconstructed.ptr);

    ast_deinit(&ast);
}
