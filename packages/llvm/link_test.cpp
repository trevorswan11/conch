#include <print>

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/MC/TargetRegistry.h>

#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/TargetSelect.h>

#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>

auto main(int argc, char** argv) -> int {
    llvm::InitLLVM    test{argc, argv};
    llvm::LLVMContext ctx;

    std::println("Built-in: {}", llvm::sys::getDefaultTargetTriple());
    std::println("Runtime:  {}", LLVM_DEFAULT_TARGET_TRIPLE);
}
