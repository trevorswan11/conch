#include <catch_amalgamated.hpp>

#include <numbers>
#include <variant>

#include "common.hpp"

TEST_CASE("Variant visitor") {
    std::variant<std::string, int, double> one_of_many;
    const std::string                      expected_string{"Hello!"};
    const int                              expected_int{42};
    const double                           expected_double{std::numbers::pi};

    const auto visitor = conch::Overloaded{
        [&expected_string](const std::string& s) { return s == expected_string; },
        [](int i) { return i == expected_int; },
        [&expected_double](double d) { return d == expected_double; },
    };

    one_of_many = expected_string;
    REQUIRE(std::visit(visitor, one_of_many));
    one_of_many = expected_int;
    REQUIRE(std::visit(visitor, one_of_many));
    one_of_many = expected_double;
    REQUIRE(std::visit(visitor, one_of_many));
}
