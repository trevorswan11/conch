//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Passes/CMakeLists.txt
pub const root = "llvm/lib/Passes";
pub const sources = [_][]const u8{
    "CodeGenPassBuilder.cpp",
    "OptimizationLevel.cpp",
    "PassBuilder.cpp",
    "PassBuilderBindings.cpp",
    "PassBuilderPipelines.cpp",
    "PassPlugin.cpp",
    "StandardInstrumentations.cpp",
};
