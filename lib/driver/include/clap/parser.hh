#pragma once

#include <iostream>
#include <ostream>

#include <CLI/App.hpp>

#include "cmd/debug.hh"
#include "platform/win32.hh"

#include <config.h>
#include <result.hh>
#include <types.hh>
#include <variant.hh>

namespace ghoti::clap {

using Parsed = Variant<Unit, cmd::Debug>;

class Parser {
  public:
    Parser(i32 argc, char** argv, std::ostream& os = std::cerr, bool ensure_utf8 = true) noexcept;

    auto               parse() -> Result<void, i32>;
    [[nodiscard]] auto get_parsed() noexcept -> Parsed& { return parsed_; }

  private:
    i32           argc_;
    char**        argv_;
    std::ostream& os_;

    CLI::App app_;
    Parsed   parsed_;

#if GHOTI_WINDOWS
    win32::RichConsole console_;
#endif
};

} // namespace ghoti::clap
