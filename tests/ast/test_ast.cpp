#include "catch_amalgamated.hpp"

#include <stdlib.h>
#include <string>

#include "parser_helpers.hpp"

extern "C" {
#include "ast/ast.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"
}

void test_reconstruction(const char* input, std::string expected) {
    ParserFixture pf(input);
    check_parse_errors(pf.parser(), {}, true);

    StringBuilder sb;
    REQUIRE(STATUS_OK(string_builder_init(&sb, expected.size())));
    REQUIRE(STATUS_OK(ast_reconstruct(pf.ast(), &sb)));

    MutSlice actual;
    REQUIRE(STATUS_OK(string_builder_to_string(&sb, &actual)));
    REQUIRE(expected == actual.ptr);
    free(actual.ptr);
}

TEST_CASE("Declaration reconstructions") {
    test_reconstruction("var my_var := another_var", "var my_var := another_var;");
}
