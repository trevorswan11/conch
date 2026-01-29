#include <algorithm>
#include <cctype>
#include <iostream>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

#include "cli/program.hpp"

namespace conch {

auto Program::repl() const -> void {
    const auto trim = [](std::string_view s) -> std::string_view {
        const auto is_space = [](unsigned char c) { return std::isspace(c); };
        const auto first    = std::ranges::find_if_not(s, is_space);
        const auto last     = std::ranges::find_if_not(s | std::views::reverse, is_space).base();

        return first < last ? std::string_view(first, last - first) : std::string_view{};
    };

    std::println("Welcome to Conch REPL! Type 'exit' to quit.");
    std::string line;
    while (true) {
        std::print(">>> ");
        line.clear();

        std::getline(std::cin, line);
        const auto trimmed = trim(line);
        if (trimmed == "exit") { break; }
    }
}

} // namespace conch
