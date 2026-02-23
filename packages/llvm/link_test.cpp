#include <print>

#include "llvm/Support/InitLLVM.h"

int main(int argc, char** argv) {
    llvm::InitLLVM test{argc, argv};
    std::println("Hello, World!");
}
