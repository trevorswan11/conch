#pragma once

#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <CLI/Formatter.hpp>

namespace ghoti::clap {

class Fmt : public CLI::Formatter {
  public:
    Fmt() noexcept = default;

    [[nodiscard]] auto make_subcommands(const CLI::App* app, CLI::AppFormatMode) const
        -> std::string override;

    [[nodiscard]] auto
    make_group(std::string group, bool is_positional, std::vector<const CLI::Option*> opts) const
        -> std::string override;

    [[nodiscard]] auto make_help(const CLI::App*    app,
                                 std::string        name,
                                 CLI::AppFormatMode mode) const -> std::string override;
};

} // namespace ghoti::clap
