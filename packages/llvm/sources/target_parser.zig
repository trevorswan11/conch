//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TargetParser/CMakeLists.txt
pub const root = "llvm/lib/TargetParser";
pub const sources = [_][]const u8{
    "AArch64TargetParser.cpp",
    "ARMTargetParserCommon.cpp",
    "ARMTargetParser.cpp",
    "CSKYTargetParser.cpp",
    "Host.cpp",
    "LoongArchTargetParser.cpp",
    "PPCTargetParser.cpp",
    "RISCVISAInfo.cpp",
    "RISCVTargetParser.cpp",
    "SubtargetFeature.cpp",
    "TargetParser.cpp",
    "Triple.cpp",
    "X86TargetParser.cpp",
};
