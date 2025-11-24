#include "catch_amalgamated.hpp"

#include <stdlib.h>

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/expression.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/type.h"
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
    REQUIRE(STATUS_OK(ast_init(&ast, standard_allocator)));

    IdentifierExpression* ident_lhs;
    Slice                 my_literal = slice_from_str_z("my_var");
    REQUIRE(STATUS_OK(identifier_expression_create(
        token_init(TokenType::IDENT, my_literal.ptr, my_literal.length, 0, 0),
        &ident_lhs,
        malloc,
        free)));

    IdentifierExpression* ident_rhs;
    Slice                 another_literal = slice_from_str_z("another_var");
    REQUIRE(STATUS_OK(identifier_expression_create(
        token_init(TokenType::IDENT, another_literal.ptr, another_literal.length, 0, 0),
        &ident_rhs,
        malloc,
        free)));

    TypeExpression* type;
    REQUIRE(STATUS_OK(type_expression_create(token_init(TokenType::COLON, "", 0, 0, 0),
                                             TypeTag::IMPLICIT,
                                             IMPLICIT_TYPE,
                                             &type,
                                             malloc)));

    DeclStatement* decl;
    REQUIRE(STATUS_OK(decl_statement_create(
        token_init(TokenType::VAR, KEYWORD_VAR.slice.ptr, KEYWORD_VAR.slice.length, 0, 0),
        ident_lhs,
        type,
        (Expression*)ident_rhs,
        &decl,
        malloc)));

    Statement* stmt = (Statement*)decl;
    array_list_push_assume_capacity(&ast.statements, &stmt);

    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, 30)));
    REQUIRE(STATUS_OK(ast_reconstruct(&ast, &sb)));

    MutSlice reconstructed;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &reconstructed)));
    REQUIRE(reconstructed.ptr);
    REQUIRE(mut_slice_equals_str_z(&reconstructed, "var my_var := another_var;"));
    free(reconstructed.ptr);

    ast_deinit(&ast);
}
