//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target
pub const root = "llvm/lib/Target";
pub const base_sources = [_][]const u8{
    "RegisterTargetPassConfigCallback.cpp",
    "Target.cpp",
    "TargetLoweringObjectFile.cpp",
    "TargetMachine.cpp",
    "TargetMachineC.cpp",
};
