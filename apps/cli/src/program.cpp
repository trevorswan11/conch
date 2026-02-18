#include <iostream>
#include <print>
#include <string>

#include "program.hpp"

#include "lexer/lexer.hpp"

#include "string.hpp"

namespace conch::cli {

auto Program::repl() -> void {
    Lexer lexer;

    std::println("Welcome to Conch REPL! Type 'exit' to quit.");
    std::string line;
    while (true) {
        std::print(">>> ");
        line.clear();

        if (!std::getline(std::cin, line)) { break; }
        const auto trimmed = string::trim(line);
        if (trimmed == "exit") { break; }

        lexer.reset(trimmed);
        for (const auto& token : lexer) { std::println("{}", token); }
    }
}

} // namespace conch::cli
