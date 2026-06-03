#include "cmd/debug.hh"

#include <filesystem>
#include <iostream>
#include <string>

#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

#include "ast/dumper.hh"
#include "module/memory_loader.hh"
#include "module/module.hh"
#include "sema/analyzer.hh"

#include "string.hh"

namespace ghoti::cmd {

auto Debug::run() -> void {
    const std::filesystem::path stdin_path = "stdin.gh";
    while (true) {
        fmt::print(">>> ");
        line_.clear();

        if (!std::getline(std::cin, line_)) { break; }
        auto trimmed = string::trim(line_);
        if (trimmed == "exit") { break; }
        if (trimmed.empty()) { continue; }

        mod::MemoryLoader  loader;
        mod::ModuleManager manager{loader};
        loader.add(stdin_path, std::string{trimmed});

        sema::Analyzer analyzer{manager, std::cerr, true};
        if (!analyzer.analyze(stdin_path)) {
            fmt::println(std::cerr, "Failed to load input from stdin");
            break;
        }

        // Print debug information from each stage
        const auto stdin_mod = *manager.try_get_file_module(stdin_path);
        if (stdin_mod->is_errored()) { continue; }

        ast::ASTDumper dumper{stdin_mod->ast, std::cout};
        for (const auto& node : stdin_mod->ast) { dumper.dump(node); }

        if (stdin_mod->is_poisoned()) { continue; }
        fmt::println("{} total tables, {} top-level symbols collected",
                     analyzer.get_registry().size(),
                     analyzer.get_table(*stdin_mod->root_table_idx).size());
    }
}

} // namespace ghoti::cmd
