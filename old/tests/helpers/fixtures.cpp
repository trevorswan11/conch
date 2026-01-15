#include "catch_amalgamated.hpp"

#include <iostream>

#include "fixtures.hpp"

auto check_errors(const ArrayList*             actual_errors,
                  std::span<const std::string> expected_errors,
                  bool                         print_anyways) -> void {
    MutSlice error;
    if (actual_errors->length == 0 && expected_errors.empty()) { return; }
    if (print_anyways && actual_errors->length != 0) {
        for (size_t i = 0; i < actual_errors->length; i++) {
            REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
            std::cerr << "Error: " << error.ptr << "\n";
        }
    }

    REQUIRE(actual_errors->length == expected_errors.size());

    for (size_t i = 0; i < actual_errors->length; i++) {
        REQUIRE(STATUS_OK(array_list_get(actual_errors, i, &error)));
        std::string expected = expected_errors[i];
        REQUIRE(expected == error.ptr);
    }
}

ParserFixture::ParserFixture(const char* input) : m_IO(file_io_std()) {
    REQUIRE(STATUS_OK(lexer_init(&m_Lexer, input, &std_allocator)));
    REQUIRE(STATUS_OK(lexer_consume(&m_Lexer)));

    REQUIRE(STATUS_OK(ast_init(&m_AST, &std_allocator)));

    REQUIRE(STATUS_OK(parser_init(&m_Parser, &m_Lexer, &m_IO, &std_allocator)));
    REQUIRE(STATUS_OK(parser_consume(&m_Parser, &m_AST)));
}

auto ParserFixture::check_errors(std::span<const std::string> expected_errors) const -> void {
    const bool empty = expected_errors.empty();
    check_errors(expected_errors, empty);
}

auto ParserFixture::check_errors(std::span<const std::string> expected_errors,
                                 bool                         print_anyways) const -> void {
    ::check_errors(&m_Parser.errors, expected_errors, print_anyways);
}

SemanticFixture::SemanticFixture(const char* input, Allocator* allocator) : m_PF{input} {
    m_PF.check_errors();
    REQUIRE(STATUS_OK(seman_init(m_PF.ast(), &m_Sema, allocator)));
    REQUIRE(STATUS_OK(seman_analyze(&m_Sema)));
}

auto SemanticFixture::check_errors(std::span<const std::string> expected_errors) const -> void {
    const bool empty = expected_errors.empty();
    check_errors(expected_errors, empty);
}

auto SemanticFixture::check_errors(std::span<const std::string> expected_errors,
                                   bool                         print_anyways) const -> void {
    m_PF.check_errors();
    ::check_errors(&m_Sema.errors, expected_errors, print_anyways);
}

SBFixture::SBFixture(size_t initial_length) {
    REQUIRE(STATUS_OK(string_builder_init(&m_Builder, initial_length)));
}

auto SBFixture::to_string() -> char* {
    MutSlice out;
    REQUIRE(STATUS_OK(string_builder_to_string(&m_Builder, &out)));
    return out.ptr;
}

CStringFixture::CStringFixture(const char* source) {
    const auto src_slice = slice_from_str_z(source);
    MutSlice   dupe;
    REQUIRE(STATUS_OK(slice_dupe(&dupe, &src_slice, &std_allocator)));
    m_Buffer = dupe.ptr;
}
