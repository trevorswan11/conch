#include "clap/parser.hh"

#include <iostream>

#include <CLI/CLI.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include "clap/formatter.hh"
#include "cmd/debug.hh"

#include <assert.hh>
#include <config.h>
#include <memory.hh>
#include <result.hh>
#include <style.hh>
#include <types.hh>

namespace ghoti::clap {

Parser::Parser(i32 argc, char** argv, std::ostream& os, bool ensure_utf8) noexcept
    : argc_{argc}, os_{os} {
    ASSERT(argc > 0, "The program name must be present");
    app_.formatter(mem::make_rc<Fmt>());
    argv_ = ensure_utf8 ? app_.ensure_utf8(argv) : argv;
}

auto Parser::parse() -> Result<void, i32> {
    app_.usage("Usage: ghoti [command] [options]");
    app_.set_version_flag("-v,--version",
                          fmt::format("ghoti v{} ({})", GHOTI_VERSION_STR, GHOTI_GIT_INFO));
    app_.require_subcommand(1);

    const auto* ast_app = app_.add_subcommand("debug", "Run the CLI interactive debugger");

    // No arguments should be handled by printing help an exiting
    if (argc_ == 1) {
        fmt::println(os_, "{}", app_.help());
        os_ << fmt::format(style::RED_BOLD, "error");
        fmt::println(os_, ": expected command argument");
        return Err{1};
    }

    try {
        app_.parse(argc_, argv_);
    } catch (const CLI::ParseError& e) { return Err{app_.exit(e)}; };
    if (ast_app->parsed()) { parsed_.emplace<cmd::Debug>(); }

    return {};
}

} // namespace ghoti::clap
