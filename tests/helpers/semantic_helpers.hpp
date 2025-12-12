#pragma once

#include <vector>

#include "parser_helpers.hpp"

extern "C" {
#include "semantic/analyzer.h"
}

class SemanticFixture {
  public:
    SemanticFixture(const char* input);

    ~SemanticFixture() { seman_deinit(&seman); }

    SemanticAnalyzer* analyzer() { return &seman; }
    Parser*           parser() { return pf.parser(); }
    AST*              ast() { return pf.ast(); }
    Lexer*            lexer() { return pf.lexer(); }

    void check_errors(std::vector<std::string> expected_errors, bool print_anyways = false);

  private:
    SemanticAnalyzer seman;
    ParserFixture    pf;
};

void test_analyze(const char*              input,
                  std::vector<std::string> expected_errors = {},
                  bool                     print_anyways   = false);
