//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/InstallAPI/CMakeLists.txt
pub const root = "clang/lib/InstallAPI";
pub const sources = [_][]const u8{
    "DiagnosticBuilderWrappers.cpp",
    "DirectoryScanner.cpp",
    "DylibVerifier.cpp",
    "FileList.cpp",
    "Frontend.cpp",
    "HeaderFile.cpp",
    "Library.cpp",
    "Visitor.cpp",
};
