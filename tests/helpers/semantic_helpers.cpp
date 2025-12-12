#include "catch_amalgamated.hpp"

#include "semantic_helpers.hpp"

extern "C" {
#include "util/allocator.h"
#include "util/status.h"
}

SemanticFixture::SemanticFixture(const char* input) : pf(input) {
    pf.check_errors({}, true);
    REQUIRE(STATUS_OK(seman_init(pf.ast(), &seman, standard_allocator)));
    REQUIRE(STATUS_OK(seman_analyze(&seman)));
}

void SemanticFixture::check_errors(std::vector<std::string> expected_errors, bool print_anyways) {
    pf.check_errors({}, true);
    ::check_errors(&seman.errors, expected_errors, print_anyways);
}

void test_analyze(const char* input, std::vector<std::string> expected_errors, bool print_anyways) {
    SemanticFixture sf(input);
    sf.check_errors(expected_errors, print_anyways);
}
