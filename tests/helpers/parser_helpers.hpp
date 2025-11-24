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

class ParserFixture {
  public:
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

  private:
    Parser p;
    AST    a;
    Lexer  l;
    FileIO stdio;
};

void check_parse_errors(Parser*                  p,
                        std::vector<std::string> expected_errors,
                        bool                     print_anyways = false);

// This does not verify any correctness for non-identifiers.
void test_type_expression(Expression*     expression,
                          bool            expect_nullable,
                          bool            expect_primitive,
                          std::string     expected_type_literal,
                          TypeTag         expected_tag,
                          ExplicitTypeTag expected_explicit_tag,
                          std::string     expected_type_name);
void test_decl_statement(Statement* stmt, bool expect_const, std::string expected_ident);

// This does not verify any correctness for non-identifiers.
void test_decl_statement(Statement*      stmt,
                         bool            expect_const,
                         std::string     expected_ident,
                         bool            expect_nullable,
                         bool            expect_primitive,
                         std::string     expected_type_literal,
                         TypeTag         expected_tag,
                         ExplicitTypeTag expected_explicit_tag,
                         std::string     expected_type_name);

template <typename T>
void test_number_expression(Expression* expression, const char* expected_literal, T expected_value);

template <typename T>
void test_number_expression(const char* input, const char* expected_literal, T expected_value);

void test_bool_expression(Expression* expression,
                          bool        expected_value,
                          std::string expected_literal);

void test_string_expression(Expression* expression,
                            std::string expected_string_literal,
                            std::string expected_token_literal);
void test_identifier_expression(Expression* expression,
                                std::string expected_name,
                                TokenType   expected_type = TokenType::IDENT);
