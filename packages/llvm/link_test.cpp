#include <print>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/InitLLVM.h>

auto main(int argc, char** argv) -> int {
    llvm::InitLLVM    test{argc, argv};
    llvm::LLVMContext ctx;
    std::println("Hello, World!");
}
