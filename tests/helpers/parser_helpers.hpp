#pragma once

#include <string>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "ast/expressions/type.h"
#include "ast/statements/statement.h"

#include "lexer/lexer.h"
#include "parser/parser.h"

#include "util/containers/string_builder.h"
}

class SBFixture {
  public:
    SBFixture(size_t initial_length);
    ~SBFixture() { free(builder.buffer.data); }

    StringBuilder* sb() { return &builder; }
    char*          to_string();

  private:
    StringBuilder builder;
};

class ParserFixture {
  public:
    ParserFixture(const char* input);

    ~ParserFixture() {
        parser_deinit(&p);
        ast_deinit(&a);
        lexer_deinit(&l);
    }

    Parser* parser() { return &p; }
    AST*    ast() { return &a; }
    Lexer*  lexer() { return &l; }

    void check_errors(std::vector<std::string> expected_errors = {});
    void check_errors(std::vector<std::string> expected_errors, bool print_anyways);

  private:
    Parser p;
    AST    a;
    Lexer  l;
    FileIO stdio;
};

void check_errors(const ArrayList*         actual_errors,
                  std::vector<std::string> expected_errors,
                  bool                     print_anyways);

void test_reconstruction(const char* input, std::string expected);

// This does not verify any correctness for non-identifiers.
void test_type_expression(Expression*     expression,
                          bool            expect_nullable,
                          bool            expect_primitive,
                          TypeTag         expected_tag,
                          ExplicitTypeTag expected_explicit_tag,
                          std::string     expected_type_literal);
void test_decl_statement(Statement* stmt, bool expect_const, std::string expected_ident);

// This does not verify any correctness for non-identifiers.
void test_decl_statement(Statement*      stmt,
                         bool            expect_const,
                         std::string     expected_ident,
                         bool            expect_nullable,
                         bool            expect_primitive,
                         TypeTag         expected_tag,
                         ExplicitTypeTag expected_explicit_tag,
                         std::string     expected_type_literal);

template <typename T> void test_number_expression(Expression* expression, T expected_value);
template <typename T> void test_number_expression(const char* input, T expected_value);

void test_bool_expression(Expression* expression, bool expected_value);

void test_string_expression(Expression* expression, std::string expected_string_literal);
void test_identifier_expression(Expression* expression,
                                std::string expected_name,
                                TokenType   expected_type = TokenType::IDENT);
