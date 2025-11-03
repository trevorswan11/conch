#include "test_framework/catch_amalgamated.hpp"

extern "C" {
#include "welcome.h"
}

TEST_CASE("Add") {
    REQUIRE(add(2, 3) == 5);
}
