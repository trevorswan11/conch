#include "catch_amalgamated.hpp"

#include <iostream>

#include "fixtures.hpp"

void check_errors(const ArrayList*         actual_errors,
                  std::vector<std::string> expected_errors,
                  bool                     print_anyways) {
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

ParserFixture::ParserFixture(const char* input) : stdio(file_io_std()) {
    REQUIRE(STATUS_OK(lexer_init(&l, input, standard_allocator)));
    REQUIRE(STATUS_OK(lexer_consume(&l)));

    REQUIRE(STATUS_OK(ast_init(&a, standard_allocator)));

    REQUIRE(STATUS_OK(parser_init(&p, &l, &stdio, standard_allocator)));
    REQUIRE(STATUS_OK(parser_consume(&p, &a)));
}

void ParserFixture::check_errors(std::vector<std::string> expected_errors) {
    const bool empty = expected_errors.empty();
    check_errors(std::move(expected_errors), empty);
}

void ParserFixture::check_errors(std::vector<std::string> expected_errors, bool print_anyways) {
    ::check_errors(&p.errors, std::move(expected_errors), print_anyways);
}

SemanticFixture::SemanticFixture(const char* input) : pf{input} {
    pf.check_errors();
    REQUIRE(STATUS_OK(seman_init(pf.ast(), &seman, standard_allocator)));
    REQUIRE(STATUS_OK(seman_analyze(&seman)));
}

void SemanticFixture::check_errors(std::vector<std::string> expected_errors) {
    const bool empty = expected_errors.empty();
    check_errors(std::move(expected_errors), empty);
}

void SemanticFixture::check_errors(std::vector<std::string> expected_errors, bool print_anyways) {
    pf.check_errors();
    ::check_errors(&seman.errors, std::move(expected_errors), print_anyways);
}

SBFixture::SBFixture(size_t initial_length) {
    REQUIRE(STATUS_OK(string_builder_init(&builder, initial_length)));
}

char* SBFixture::to_string() {
    MutSlice out;
    REQUIRE(STATUS_OK(string_builder_to_string(&builder, &out)));
    return out.ptr;
}

CStringFixture::CStringFixture(const char* source) {
    const auto src_slice = slice_from_str_z(source);
    MutSlice   dupe;
    REQUIRE(STATUS_OK(slice_dupe(&dupe, &src_slice, malloc)));
    buffer = dupe.ptr;
}
