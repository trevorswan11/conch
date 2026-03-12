// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Demangle/CMakeLists.txt
pub const root = "llvm/lib/Demangle";
pub const sources = [_][]const u8{
    "DLangDemangle.cpp",
    "Demangle.cpp",
    "ItaniumDemangle.cpp",
    "MicrosoftDemangle.cpp",
    "MicrosoftDemangleNodes.cpp",
    "RustDemangle.cpp",
};
