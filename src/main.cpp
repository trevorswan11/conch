#include <print>

auto main() -> int {
    try {
        std::println("Hello, World!");
    } catch (...) { return 1; }
    return 0;
}
