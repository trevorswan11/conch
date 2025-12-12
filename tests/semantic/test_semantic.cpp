#include "catch_amalgamated.hpp"

#include <vector>

#include "semantic_helpers.hpp"

TEST_CASE("Const / var declarations") {
    SECTION("Implicit constants") {
        const std::vector<const char*> inputs = {
            "const v := 5;",
        };

        for (const auto& i : inputs) {
            test_analyze(i);
        }
    }
}
