//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/LTO/CMakeLists.txt
pub const root = "llvm/lib/LTO";
pub const sources = [_][]const u8{
    "LTO.cpp",
    "LTOBackend.cpp",
    "LTOModule.cpp",
    "LTOCodeGenerator.cpp",
    "UpdateCompilerUsed.cpp",
    "ThinLTOCodeGenerator.cpp",
};
