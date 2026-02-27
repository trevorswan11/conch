//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TextAPI/CMakeLists.txt
pub const root = "llvm/lib/TextAPI/";
pub const sources = [_][]const u8{
    "Architecture.cpp",
    "ArchitectureSet.cpp",
    "InterfaceFile.cpp",
    "TextStubV5.cpp",
    "PackedVersion.cpp",
    "Platform.cpp",
    "RecordsSlice.cpp",
    "RecordVisitor.cpp",
    "Symbol.cpp",
    "SymbolSet.cpp",
    "Target.cpp",
    "TextAPIError.cpp",
    "TextStub.cpp",
    "TextStubCommon.cpp",
    "Utils.cpp",
};

pub const binary_reader = root ++ "BinaryReader/DylibReader.cpp";
