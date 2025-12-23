#pragma once

#include <vector>

#include "parser_helpers.hpp"

extern "C" {
#include "semantic/analyzer.h"
}

class SemanticFixture {
  public:
    explicit SemanticFixture(const char* input);

    ~SemanticFixture() { seman_deinit(&seman); }

    const SemanticAnalyzer* analyzer() { return &seman; }
    const Parser*           parser() { return pf.parser(); }
    const AST*              ast() { return pf.ast(); }
    const Lexer*            lexer() { return pf.lexer(); }

    void check_errors(std::vector<std::string> expected_errors = {});
    void check_errors(std::vector<std::string> expected_errors, bool print_anyways);

  private:
    SemanticAnalyzer seman;
    ParserFixture    pf;
};

void test_analyze(const char* input, std::vector<std::string> expected_errors = {});
void test_analyze(const char* input, std::vector<std::string> expected_errors, bool print_anyways);
