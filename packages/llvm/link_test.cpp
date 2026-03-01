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

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string TargetTriple  = llvm::sys::getDefaultTargetTriple();
    std::string BuiltInTriple = LLVM_DEFAULT_TARGET_TRIPLE;

    std::println("Built-in: {}", BuiltInTriple);
    std::println("Runtime:  {}", TargetTriple);
}
