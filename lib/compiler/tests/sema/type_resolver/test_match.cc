#include <catch2/catch_test_macros.hpp>

#include "helpers/sema.hh"

namespace ghoti::tests {

TEST_CASE("Standard match expression") {
    helpers::resolve_and_check(R"(
        match (23) {
            1 => "asdf",
            2 => false,
        };
    )");
}

} // namespace ghoti::tests
