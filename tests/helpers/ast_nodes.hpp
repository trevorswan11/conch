#pragma once

#include <string>

extern "C" {
#include "ast/expressions/type.h"
#include "ast/statements/statement.h"
}

// This does not verify any correctness for non-identifiers.
void test_type_expression(const Expression* expression,
                          bool              expect_nullable,
                          bool              expect_primitive,
                          TypeExpressionTag expected_tag,
                          ExplicitTypeTag   expected_explicit_tag,
                          std::string       expected_type_literal);
void test_decl_statement(const Statement* stmt, bool expect_const, std::string expected_ident);

// This does not verify any correctness for non-identifiers.
void test_decl_statement(const Statement*  stmt,
                         bool              expect_const,
                         std::string       expected_ident,
                         bool              expect_nullable,
                         bool              expect_primitive,
                         TypeExpressionTag expected_tag,
                         ExplicitTypeTag   expected_explicit_tag,
                         std::string       expected_type_literal);

template <typename T> void test_number_expression(const Expression* expression, T expected_value);
template <typename T> void test_number_expression(const char* input, T expected_value);

void test_bool_expression(const Expression* expression, bool expected_value);

void test_string_expression(const Expression* expression, std::string expected_string_literal);
void test_identifier_expression(const Expression* expression,
                                std::string       expected_name,
                                TokenType         expected_type = TokenType::IDENT);
