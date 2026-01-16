#include <catch_amalgamated.hpp>

#include <string>

#include "core.hpp"

enum class TestEnum {
    FIRST,
    SECOND,
    LAST      = 23,
    FAKE_LAST = 22,
};

TEST_CASE("Compile time enum names") {
    REQUIRE(enum_name<TestEnum::FIRST>() == std::string{"FIRST"});
    REQUIRE(enum_name<TestEnum::SECOND>() == std::string{"SECOND"});
    REQUIRE(enum_name<TestEnum::LAST>() == std::string{"LAST"});
    REQUIRE(enum_name<TestEnum::FAKE_LAST>() == std::string{"FAKE_LAST"});
    REQUIRE_FALSE(enum_name<TestEnum::FAKE_LAST>() == std::string{"LAST"});
}

TEST_CASE("Runtime enum names") {
    REQUIRE(enum_name(TestEnum::FIRST) == std::string{"FIRST"});
    REQUIRE(enum_name(TestEnum::SECOND) == std::string{"SECOND"});
    REQUIRE(enum_name(TestEnum::LAST) == std::string{"LAST"});
    REQUIRE(enum_name(TestEnum::FAKE_LAST) == std::string{"FAKE_LAST"});
    REQUIRE_FALSE(enum_name(TestEnum::FAKE_LAST) == std::string{"LAST"});
}
