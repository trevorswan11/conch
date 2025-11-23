#include "catch_amalgamated.hpp"

#include <iostream>

#include "parser_helpers.hpp"

extern "C" {
#include "ast/expressions/bool.h"
#include "ast/expressions/float.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/string.h"
#include "ast/node.h"
#include "ast/statements/declarations.h"
}

ParserFixture::ParserFixture(const char* input) : stdio(file_io_std()) {
    REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
    REQUIRE(STATUS_OK(lexer_consume(&l)));

    REQUIRE(STATUS_OK(ast_init(&a, standard_allocator)));

    REQUIRE(STATUS_OK(parser_init(&p, &l, &stdio, standard_allocator)));
    REQUIRE(STATUS_OK(parser_consume(&p, &a)));
}

void check_parse_errors(Parser* p, std::vector<std::string> expected_errors, bool print_anyways) {
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

void test_decl_statement(Statement* stmt, bool expect_const, const char* expected_ident) {
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

void test_decl_statement(Statement*  stmt,
                         bool        expect_const,
                         const char* expected_ident,
                         bool        expect_nullable,
                         bool        expect_primitive,
                         const char* expected_type_literal,
                         TypeTag     expected_tag,
                         const char* expected_type_name) {
    Node* stmt_node = (Node*)stmt;
    Slice literal   = stmt_node->vtable->token_literal(stmt_node);
    REQUIRE(slice_equals_str_z(&literal, expect_const ? "const" : "var"));

    DeclStatement*        decl_stmt = (DeclStatement*)stmt;
    IdentifierExpression* ident     = decl_stmt->ident;
    Node*                 node      = (Node*)ident;
    literal                         = node->vtable->token_literal(node);

    REQUIRE(slice_equals_str_z(&literal, token_type_name(TokenType::IDENT)));
    REQUIRE(mut_slice_equals_str_z(&ident->name, expected_ident));

    TypeExpression* type_expr = (TypeExpression*)decl_stmt->type;
    Type            type      = type_expr->type;

    REQUIRE(type.tag == expected_tag);
    if (type.tag == TypeTag::EXPLICIT) {
        const ExplicitType explicit_type = type.variant.explicit_type;
        REQUIRE(explicit_type.nullable == expect_nullable);
        REQUIRE(explicit_type.primitive == expect_primitive);

        ident   = explicit_type.identifier;
        node    = (Node*)ident;
        literal = node->vtable->token_literal(node);

        REQUIRE(slice_equals_str_z(&literal, expected_type_literal));
        REQUIRE(mut_slice_equals_str_z(&ident->name, expected_type_name));
    } else {
        REQUIRE_FALSE(expect_nullable);
        REQUIRE_FALSE(expect_primitive);
        REQUIRE_FALSE(expected_type_name);
    }
}

template <typename T>
void test_number_expression(Expression* expression,
                            const char* expected_literal,
                            T           expected_value) {
    if constexpr (std::is_same_v<T, double>) {
        FloatLiteralExpression* f = (FloatLiteralExpression*)expression;
        REQUIRE(f->value == expected_value);
        Node* f_node = (Node*)f;
        if (expected_literal) {
            Slice literal = f_node->vtable->token_literal(f_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else if constexpr (std::is_same_v<T, int64_t>) {
        IntegerLiteralExpression* i = (IntegerLiteralExpression*)expression;
        REQUIRE(i->value == expected_value);
        Node* i_node = (Node*)i;
        if (expected_literal) {
            Slice literal = i_node->vtable->token_literal(i_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        UnsignedIntegerLiteralExpression* i = (UnsignedIntegerLiteralExpression*)expression;
        REQUIRE(i->value == expected_value);
        Node* i_node = (Node*)i;
        if (expected_literal) {
            Slice literal = i_node->vtable->token_literal(i_node);
            REQUIRE(slice_equals_str_z(&literal, expected_literal));
        }
    } else {
        REQUIRE(false);
    }
}

template void test_number_expression<int64_t>(Expression*, const char*, int64_t);
template void test_number_expression<uint64_t>(Expression*, const char*, uint64_t);
template void test_number_expression<double>(Expression*, const char*, double);

template <typename T>
void test_number_expression(const char* input, const char* expected_literal, T expected_value) {
    ParserFixture pf(input);
    auto          ast = pf.ast();

    check_parse_errors(&pf.p, {}, true);
    REQUIRE(ast->statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, &stmt)));

    ExpressionStatement* expr = (ExpressionStatement*)stmt;
    test_number_expression<T>(expr->expression, expected_literal, expected_value);
}

template void test_number_expression<int64_t>(const char*, const char*, int64_t);
template void test_number_expression<uint64_t>(const char*, const char*, uint64_t);
template void test_number_expression<double>(const char*, const char*, double);

void test_bool_expression(Expression* expression,
                          bool        expected_value,
                          const char* expected_literal) {
    BoolLiteralExpression* boolean = (BoolLiteralExpression*)expression;
    REQUIRE(boolean->value == expected_value);

    Node* bool_node = (Node*)boolean;
    Slice literal   = bool_node->vtable->token_literal(bool_node);
    REQUIRE(slice_equals_str_z(&literal, expected_literal));
}

void test_string_expression(Expression* expression,
                            std::string expected_string_literal,
                            std::string expected_token_literal) {
    StringLiteralExpression* string = (StringLiteralExpression*)expression;
    REQUIRE(expected_string_literal == string->slice.ptr);

    std::string actual_token_literal(string->token.slice.ptr, string->token.slice.length);
    REQUIRE(expected_token_literal == actual_token_literal);
}

void test_identifier_expression(Expression* expression,
                                std::string expected_name,
                                TokenType   expected_type) {
    IdentifierExpression* ident = (IdentifierExpression*)expression;
    REQUIRE(expected_name == ident->name.ptr);
    REQUIRE(expected_type == ident->token_type);
}
