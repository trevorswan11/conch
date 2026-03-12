//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Remarks/CMakeLists.txt
pub const root = "llvm/lib/Remarks";
pub const sources = [_][]const u8{
    "BitstreamRemarkParser.cpp",
    "BitstreamRemarkSerializer.cpp",
    "Remark.cpp",
    "RemarkFormat.cpp",
    "RemarkLinker.cpp",
    "RemarkParser.cpp",
    "RemarkSerializer.cpp",
    "RemarkStreamer.cpp",
    "RemarkStringTable.cpp",
    "YAMLRemarkParser.cpp",
    "YAMLRemarkSerializer.cpp",
};
