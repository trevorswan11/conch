//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/SandboxIR/CMakeLists.txt
pub const root = "llvm/lib/SandboxIR";
pub const sources = [_][]const u8{
    "Argument.cpp",
    "BasicBlock.cpp",
    "Constant.cpp",
    "Context.cpp",
    "Function.cpp",
    "Instruction.cpp",
    "Module.cpp",
    "Pass.cpp",
    "PassManager.cpp",
    "Region.cpp",
    "Tracker.cpp",
    "Type.cpp",
    "User.cpp",
    "Use.cpp",
    "Value.cpp",
};
