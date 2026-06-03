#include <numbers>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "types.hh"
#include "variant.hh"

namespace ghoti::tests {

TEST_CASE("Variant visitor") {
    std::variant<std::string, i32, f64> one_of_many;
    const std::string                   expected_string{"Hello!"};
    const i32                           expected_int{42};
    const f64                           expected_double{std::numbers::pi};

    const auto visitor = Overloaded{
        [&expected_string](const std::string& s) { return s == expected_string; },
        [](i32 i) { return i == expected_int; },
        [&expected_double](f64 d) { return d == expected_double; },
    };

    one_of_many = expected_string;
    CHECK(std::visit(visitor, one_of_many));
    one_of_many = expected_int;
    CHECK(std::visit(visitor, one_of_many));
    one_of_many = expected_double;
    CHECK(std::visit(visitor, one_of_many));
}

} // namespace ghoti::tests
