#include "catch_amalgamated.hpp"

#include "ast_nodes.hpp"
#include "fixtures.hpp"

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/bool.h"
#include "ast/expressions/float.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/integer.h"
#include "ast/expressions/string.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
}

void test_type_expression(const Expression* expression,
                          bool              expect_nullable,
                          bool              expect_primitive,
                          TypeExpressionTag expected_tag,
                          ExplicitTypeTag   expected_explicit_tag,
                          std::string       expected_type_literal) {
    const auto* type_expr = reinterpret_cast<const TypeExpression*>(expression);
    REQUIRE(type_expr->tag == expected_tag);

    if (type_expr->tag == TypeExpressionTag::EXPLICIT) {
        const ExplicitType explicit_type = type_expr->variant.explicit_type;
        REQUIRE(explicit_type.nullable == expect_nullable);
        REQUIRE(explicit_type.primitive == expect_primitive);
        REQUIRE(explicit_type.tag == expected_explicit_tag);

        switch (explicit_type.tag) {
        case EXPLICIT_IDENT: {
            IdentifierExpression* ident = explicit_type.variant.ident_type.name;
            REQUIRE(ident->name.ptr == expected_type_literal);
            break;
        }
        default:
            break;
        }
    } else {
        REQUIRE_FALSE(expect_nullable);
        REQUIRE_FALSE(expect_primitive);
        REQUIRE(expected_type_literal.empty());
    }
}

void test_decl_statement(const Statement* stmt, bool expect_const, std::string expected_ident) {
    const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);
    REQUIRE(expect_const == decl_stmt->is_const);
    REQUIRE(expected_ident == decl_stmt->ident->name.ptr);
}

void test_decl_statement(const Statement*  stmt,
                         bool              expect_const,
                         std::string       expected_ident,
                         bool              expect_nullable,
                         bool              expect_primitive,
                         TypeExpressionTag expected_tag,
                         ExplicitTypeTag   expected_explicit_tag,
                         std::string       expected_type_literal) {
    test_decl_statement(stmt, expect_const, std::move(expected_ident));

    const auto* decl_stmt = reinterpret_cast<const DeclStatement*>(stmt);
    test_type_expression(reinterpret_cast<const Expression*>(decl_stmt->type),
                         expect_nullable,
                         expect_primitive,
                         expected_tag,
                         expected_explicit_tag,
                         std::move(expected_type_literal));
}

template <typename T> void test_number_expression(const Expression* expression, T expected_value) {
    if constexpr (std::is_same_v<T, double>) {
        const auto* f = reinterpret_cast<const FloatLiteralExpression*>(expression);
        REQUIRE(f->value == expected_value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
        const auto* i = reinterpret_cast<const IntegerLiteralExpression*>(expression);
        REQUIRE(i->value == expected_value);
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        const auto* i = reinterpret_cast<const UnsignedIntegerLiteralExpression*>(expression);
        REQUIRE(i->value == expected_value);
    } else {
        REQUIRE(false);
    }
}

template void test_number_expression<int64_t>(const Expression*, int64_t);
template void test_number_expression<uint64_t>(const Expression*, uint64_t);
template void test_number_expression<double>(const Expression*, double);

template <typename T> void test_number_expression(const char* input, T expected_value) {
    ParserFixture pf{input};
    const auto*   ast = pf.ast();

    pf.check_errors({}, true);
    REQUIRE(ast->statements.length == 1);

    Statement* stmt;
    REQUIRE(STATUS_OK(array_list_get(&ast->statements, 0, (void*)&stmt)));

    const auto* expr = reinterpret_cast<const ExpressionStatement*>(stmt);
    test_number_expression<T>(expr->expression, expected_value);
}

template void test_number_expression<int64_t>(const char*, int64_t);
template void test_number_expression<uint64_t>(const char*, uint64_t);
template void test_number_expression<double>(const char*, double);

void test_bool_expression(const Expression* expression, bool expected_value) {
    const auto* boolean = reinterpret_cast<const BoolLiteralExpression*>(expression);
    REQUIRE(boolean->value == expected_value);
}

void test_string_expression(const Expression* expression, std::string expected_string_literal) {
    const auto* string = reinterpret_cast<const StringLiteralExpression*>(expression);
    REQUIRE(expected_string_literal == string->slice.ptr);
}

void test_identifier_expression(const Expression* expression,
                                std::string       expected_name,
                                TokenType         expected_type) {
    const auto* ident = reinterpret_cast<const IdentifierExpression*>(expression);
    REQUIRE(expected_name == ident->name.ptr);
    REQUIRE(expected_type == ((Node*)ident)->start_token.type);
}
