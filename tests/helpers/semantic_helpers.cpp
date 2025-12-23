#include "catch_amalgamated.hpp"

#include "semantic_helpers.hpp"

SemanticFixture::SemanticFixture(const char* input) : pf(input) {
    pf.check_errors();
    REQUIRE(STATUS_OK(seman_init(pf.ast(), &seman, STANDARD_ALLOCATOR)));
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

void test_analyze(const char* input, std::vector<std::string> expected_errors) {
    const bool empty = expected_errors.empty();
    test_analyze(input, std::move(expected_errors), empty);
}

void test_analyze(const char* input, std::vector<std::string> expected_errors, bool print_anyways) {
    SemanticFixture sf(input);
    sf.check_errors(std::move(expected_errors), print_anyways);
}
