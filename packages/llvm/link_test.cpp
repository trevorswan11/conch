#include <iostream>

#include "llvm/Support/InitLLVM.h"

int main(int argc, char** argv) {
    llvm::InitLLVM X(argc, argv);
    std::cout << "Hello, World!";
}
