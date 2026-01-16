#include <catch_amalgamated.hpp>

#include <string>

#include "core.hpp"

TEST_CASE("Compile time enum names") {
    enum class TestEnum {
        FIRST,
        SECOND,
        LAST      = 23,
        FAKE_LAST = 22,
    };

    REQUIRE(enum_name<TestEnum::FIRST>() == std::string{"FIRST"});
    REQUIRE(enum_name<TestEnum::SECOND>() == std::string{"SECOND"});
    REQUIRE(enum_name<TestEnum::LAST>() == std::string{"LAST"});
    REQUIRE(enum_name<TestEnum::FAKE_LAST>() == std::string{"FAKE_LAST"});
}
