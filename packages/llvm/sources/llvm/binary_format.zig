//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/BinaryFormat/CMakeLists.txt
pub const root = "llvm/lib/BinaryFormat";
pub const sources = [_][]const u8{
    "AMDGPUMetadataVerifier.cpp",
    "COFF.cpp",
    "Dwarf.cpp",
    "DXContainer.cpp",
    "ELF.cpp",
    "MachO.cpp",
    "Magic.cpp",
    "Minidump.cpp",
    "MsgPackDocument.cpp",
    "MsgPackDocumentYAML.cpp",
    "MsgPackReader.cpp",
    "MsgPackWriter.cpp",
    "Wasm.cpp",
    "XCOFF.cpp",
};
