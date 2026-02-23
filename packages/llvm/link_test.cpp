#include <print>

#include <llvm/Support/InitLLVM.h>

auto main(int argc, char** argv) -> int {
    llvm::InitLLVM test{argc, argv};
    std::println("Hello, World!");
}
