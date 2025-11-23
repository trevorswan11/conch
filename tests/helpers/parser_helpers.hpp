#pragma once

#include <cstdint>
#include <string>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/type.h"
#include "ast/statements/expression.h"
#include "ast/statements/statement.h"

#include "lexer/lexer.h"
#include "parser/parser.h"
}

struct ParserFixture {
    ParserFixture(const char* input);

    ~ParserFixture() {
        parser_deinit(&p);
        ast_deinit(&a);
        lexer_deinit(&l);
    }

    Parser* parser() {
        return &p;
    }
    AST* ast() {
        return &a;
    }
    Lexer* lexer() {
        return &l;
    }

    Parser p;
    AST    a;
    Lexer  l;
    FileIO stdio;
};

void check_parse_errors(Parser*                  p,
                        std::vector<std::string> expected_errors,
                        bool                     print_anyways = false);

void test_decl_statement(Statement* stmt, bool expect_const, const char* expected_ident);
void test_decl_statement(Statement*  stmt,
                         bool        expect_const,
                         const char* expected_ident,
                         bool        expect_nullable,
                         bool        expect_primitive,
                         const char* expected_type_literal,
                         TypeTag     expected_tag,
                         const char* expected_type_name);

template <typename T>
void test_number_expression(Expression* expression, const char* expected_literal, T expected_value);

template <typename T>
void test_number_expression(const char* input, const char* expected_literal, T expected_value);

void test_bool_expression(Expression* expression,
                          bool        expected_value,
                          const char* expected_literal);

void test_string_expression(Expression* expression,
                            std::string expected_string_literal,
                            std::string expected_token_literal);
void test_identifier_expression(Expression* expression,
                                std::string expected_name,
                                TokenType   expected_type = TokenType::IDENT);
