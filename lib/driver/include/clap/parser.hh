#pragma once

#include <iostream>
#include <ostream>

#include <CLI/App.hpp>
#include <stdx/result.hh>
#include <stdx/types.hh>
#include <stdx/variant.hh>

#include "cmd/debug.hh"
#include "platform/win32.hh"

namespace ghoti::clap {

using Parsed = stdx::variant<stdx::monostate, cmd::Debug>;

class Parser {
  public:
    Parser(i32 argc, char** argv, std::ostream& os = std::cerr, bool ensure_utf8 = true) noexcept;

    auto               parse() -> stdx::result<void, i32>;
    [[nodiscard]] auto get_parsed() noexcept -> Parsed& { return parsed_; }

  private:
    i32           argc_;
    char**        argv_;
    std::ostream& os_;

    CLI::App app_;
    Parsed   parsed_;

#if STDX_WINDOWS
    win32::RichConsole console_;
#endif
};

} // namespace ghoti::clap
