#pragma once

#include <cassert>
#include <cstdlib>
#include <functional>
#include <span>
#include <string>
#include <type_traits>

extern "C" {
#include "ast/ast.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/analyzer.h"

#include "util/containers/string_builder.h"
}

auto check_errors(const ArrayList*             actual_errors,
                  std::span<const std::string> expected_errors,
                  bool                         print_anyways) -> void;

// Behaves like a stack allocated unique pointer
template <typename T> class Fixture {
  private:
    using Dtor = std::function<void(T*)>;

  public:
    explicit Fixture(std::type_identity_t<T>& t, Dtor dtor) : m_Underlying{t}, m_Dtor{dtor} {}
    explicit Fixture(std::type_identity_t<T>& t) : m_Underlying{t}, m_Dtor{nullptr} {}

    ~Fixture() {
        if constexpr (std::is_pointer_v<T>) {
            free(m_Underlying);
            return;
        }

        assert(m_Dtor);
        m_Dtor(&m_Underlying);
    }

    auto raw() -> T& { return m_Underlying; }

  private:
    T&   m_Underlying;
    Dtor m_Dtor;
};

class ParserFixture {
  public:
    explicit ParserFixture(const char* input);

    ~ParserFixture() {
        parser_deinit(&m_Parser);
        ast_deinit(&m_AST);
        lexer_deinit(&m_Lexer);
    }

    [[nodiscard]] auto parser() const -> const Parser* { return &m_Parser; }
    [[nodiscard]] auto ast() const -> const AST* { return &m_AST; }
    auto               ast_mut() -> AST* { return &m_AST; }
    [[nodiscard]] auto lexer() const -> const Lexer* { return &m_Lexer; }

    auto check_errors(std::span<const std::string> expected_errors = {}) const -> void;
    auto check_errors(std::span<const std::string> expected_errors, bool print_anyways) const
        -> void;

  private:
    Parser m_Parser;
    AST    m_AST;
    Lexer  m_Lexer;
    FileIO m_IO;
};

class SemanticFixture {
  public:
    explicit SemanticFixture(const char* input, Allocator* allocator);

    ~SemanticFixture() { seman_deinit(&m_Sema); }

    [[nodiscard]] auto analyzer() const -> const SemanticAnalyzer* { return &m_Sema; }
    [[nodiscard]] auto parser() const -> const Parser* { return m_PF.parser(); }
    [[nodiscard]] auto ast() const -> const AST* { return m_PF.ast(); }
    [[nodiscard]] auto lexer() const -> const Lexer* { return m_PF.lexer(); }

    auto check_errors(std::span<const std::string> expected_errors = {}) const -> void;
    auto check_errors(std::span<const std::string> expected_errors, bool print_anyways) const
        -> void;

  private:
    SemanticAnalyzer m_Sema;
    ParserFixture    m_PF;
};

class SBFixture {
  public:
    explicit SBFixture(size_t initial_length);
    ~SBFixture() { free(m_Builder.buffer.data); }

    auto sb() -> StringBuilder* { return &m_Builder; }
    auto to_string() -> char*;

  private:
    StringBuilder m_Builder;
};

class CStringFixture {
  public:
    explicit CStringFixture(const char* source);
    ~CStringFixture() { free(m_Buffer); };

    auto raw() -> char* { return m_Buffer; }

  private:
    char* m_Buffer;
};
