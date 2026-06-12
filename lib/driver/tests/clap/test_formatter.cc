#include <CLI/CLI.hpp>
#include <catch2/catch_test_macros.hpp>

#include "clap/formatter.hh"

#include <memory.hh>

namespace ghoti::tests {

TEST_CASE("Formatter provides output") {
    CLI::App app;
    auto     formatter{mem::make_rc<clap::Fmt>()};
    app.formatter(formatter);
    REQUIRE(app.add_subcommand("nothing"));

    CHECK_FALSE(formatter->make_subcommands(&app, CLI::AppFormatMode::All).empty());
    CHECK_FALSE(formatter->make_group("OPTIONS", false, {}).empty());
    CHECK_FALSE(formatter->make_group("", false, {}).empty());
    CHECK_FALSE(formatter->make_help(&app, "name", CLI::AppFormatMode::All).empty());
}

} // namespace ghoti::tests
