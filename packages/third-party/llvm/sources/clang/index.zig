//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Index/CMakeLists.txt
pub const root = "clang/lib/Index";
pub const sources = [_][]const u8{
    "CommentToXML.cpp",
    "FileIndexRecord.cpp",
    "IndexBody.cpp",
    "IndexDecl.cpp",
    "IndexingAction.cpp",
    "IndexingContext.cpp",
    "IndexSymbol.cpp",
    "IndexTypeSourceInfo.cpp",
    "USRGeneration.cpp",
};
