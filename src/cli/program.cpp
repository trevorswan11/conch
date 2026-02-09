#include <algorithm>
#include <cctype>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

#include "cli/program.hpp"

#include "lexer/lexer.hpp"

namespace conch {

auto Program::repl() -> void {
    const auto trim = [](std::string_view s) -> std::string_view {
        const auto isspace = [](byte c) { return std::isspace(c); };
        const auto first   = std::ranges::find_if_not(s, isspace);
        const auto last    = std::ranges::find_if_not(s | std::views::reverse, isspace).base();

        return first < last ? std::string_view(first, last - first) : std::string_view{};
    };

    Lexer lexer;

    std::println("Welcome to Conch REPL! Type 'exit' to quit.");
    std::string line;
    while (true) {
        std::print(">>> ");
        line.clear();

        if (!std::getline(std::cin, line)) { break; }
        const auto trimmed = trim(line);
        if (trimmed == "exit") { break; }

        lexer.reset(trimmed);
        for (const auto& token : lexer) { std::println("{}", token); }
    }
}

} // namespace conch
