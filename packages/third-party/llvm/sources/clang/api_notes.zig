//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/APINotes/CMakeLists.txt
pub const root = "clang/lib/APINotes";
pub const sources = [_][]const u8{
    "APINotesManager.cpp",
    "APINotesReader.cpp",
    "APINotesTypes.cpp",
    "APINotesWriter.cpp",
    "APINotesYAMLCompiler.cpp",
};
