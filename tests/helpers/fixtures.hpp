#pragma once

#include <cassert>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <vector>

extern "C" {
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/analyzer.h"

#include "util/containers/string_builder.h"
}

void check_errors(const ArrayList*         actual_errors,
                  std::vector<std::string> expected_errors,
                  bool                     print_anyways);

// Behaves like a stack allocated unique pointer
template <typename T> class Fixture {
  private:
    using Dtor = void (*)(T*);

  public:
    explicit Fixture(std::type_identity_t<T>& t, Dtor dtor) : underlying{t}, dtor{dtor} {}
    explicit Fixture(std::type_identity_t<T>& t) : underlying{t}, dtor{nullptr} {}

    ~Fixture() {
        if constexpr (std::is_pointer_v<T>) {
            free(underlying);
            return;
        }

        assert(dtor);
        dtor(&underlying);
    }

    T& raw() { return underlying; }

  private:
    T&   underlying;
    Dtor dtor;
};

class ParserFixture {
  public:
    explicit ParserFixture(const char* input);

    ~ParserFixture() {
        parser_deinit(&p);
        ast_deinit(&a);
        lexer_deinit(&l);
    }

    const Parser* parser() { return &p; }
    const AST*    ast() { return &a; }
    AST*          ast_mut() { return &a; }
    const Lexer*  lexer() { return &l; }

    void check_errors(std::vector<std::string> expected_errors = {});
    void check_errors(std::vector<std::string> expected_errors, bool print_anyways);

  private:
    Parser p;
    AST    a;
    Lexer  l;
    FileIO stdio;
};

class SemanticFixture {
  public:
    explicit SemanticFixture(const char* input, Allocator* allocator);

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

class SBFixture {
  public:
    explicit SBFixture(size_t initial_length);
    ~SBFixture() { free(builder.buffer.data); }

    StringBuilder* sb() { return &builder; }
    char*          to_string();

  private:
    StringBuilder builder;
};

class CStringFixture {
  public:
    explicit CStringFixture(const char* source);
    ~CStringFixture() { free(buffer); };

    char* raw() { return buffer; }

  private:
    char* buffer;
};
