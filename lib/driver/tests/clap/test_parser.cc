#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include <catch2/catch_test_macros.hpp>

#include "clap/parser.hh"
#include "cmd/debug.hh"
#include "helpers/argv.hh"

#include "variant.hh"

namespace ghoti::tests {

using namespace std::string_literals;

TEST_CASE("Error with no args") {
    auto               args = helpers::MockArgv{{"ghoti"s}};
    std::ostringstream error_ss;
    clap::Parser       parser{args.argc(), args.argv(), error_ss, false};
    const auto         result = parser.parse();
    REQUIRE_FALSE(result);
    CHECK(result.error() == 1);
    CHECK_FALSE(error_ss.view().empty());
}

TEST_CASE("Ast dump parser") {
    auto         args = helpers::MockArgv{{"ghoti"s, "debug"s}};
    clap::Parser parser{args.argc(), args.argv(), std::cerr, false};
    CHECK(std::holds_alternative<Unit>(parser.get_parsed()));
    REQUIRE(parser.parse());
    CHECK(std::holds_alternative<cmd::Debug>(parser.get_parsed()));
}

} // namespace ghoti::tests
