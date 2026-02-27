//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target
pub const root = "llvm/lib/Target/";
pub const base_sources = [_][]const u8{
    "RegisterTargetPassConfigCallback.cpp",
    "Target.cpp",
    "TargetLoweringObjectFile.cpp",
    "TargetMachine.cpp",
    "TargetMachineC.cpp",
};

const BackendAction = struct {
    outfile: []const u8,
    td_args: []const []const u8,
};

pub const AArch64 = struct {
    pub const backend_root = root ++ "AArch64/";
    pub const td_filepath = backend_root ++ "AArch64.td";
    pub const backend_sources = [_][]const u8{
        "GISel/AArch64CallLowering.cpp",
        "GISel/AArch64GlobalISelUtils.cpp",
        "GISel/AArch64InstructionSelector.cpp",
        "GISel/AArch64LegalizerInfo.cpp",
        "GISel/AArch64O0PreLegalizerCombiner.cpp",
        "GISel/AArch64PreLegalizerCombiner.cpp",
        "GISel/AArch64PostLegalizerCombiner.cpp",
        "GISel/AArch64PostLegalizerLowering.cpp",
        "GISel/AArch64PostSelectOptimize.cpp",
        "GISel/AArch64RegisterBankInfo.cpp",
        "AArch64A57FPLoadBalancing.cpp",
        "AArch64AdvSIMDScalarPass.cpp",
        "AArch64Arm64ECCallLowering.cpp",
        "AArch64AsmPrinter.cpp",
        "AArch64BranchTargets.cpp",
        "AArch64CallingConvention.cpp",
        "AArch64CleanupLocalDynamicTLSPass.cpp",
        "AArch64CollectLOH.cpp",
        "AArch64CondBrTuning.cpp",
        "AArch64ConditionalCompares.cpp",
        "AArch64DeadRegisterDefinitionsPass.cpp",
        "AArch64ExpandImm.cpp",
        "AArch64ExpandPseudoInsts.cpp",
        "AArch64FalkorHWPFFix.cpp",
        "AArch64FastISel.cpp",
        "AArch64A53Fix835769.cpp",
        "AArch64FrameLowering.cpp",
        "AArch64CompressJumpTables.cpp",
        "AArch64ConditionOptimizer.cpp",
        "AArch64RedundantCopyElimination.cpp",
        "AArch64ISelDAGToDAG.cpp",
        "AArch64ISelLowering.cpp",
        "AArch64InstrInfo.cpp",
        "AArch64LoadStoreOptimizer.cpp",
        "AArch64LowerHomogeneousPrologEpilog.cpp",
        "AArch64MachineFunctionInfo.cpp",
        "AArch64MachineScheduler.cpp",
        "AArch64MacroFusion.cpp",
        "AArch64MIPeepholeOpt.cpp",
        "AArch64MCInstLower.cpp",
        "AArch64PointerAuth.cpp",
        "AArch64PostCoalescerPass.cpp",
        "AArch64PromoteConstant.cpp",
        "AArch64PBQPRegAlloc.cpp",
        "AArch64RegisterInfo.cpp",
        "AArch64SLSHardening.cpp",
        "AArch64SelectionDAGInfo.cpp",
        "AArch64SpeculationHardening.cpp",
        "AArch64StackTagging.cpp",
        "AArch64StackTaggingPreRA.cpp",
        "AArch64StorePairSuppress.cpp",
        "AArch64Subtarget.cpp",
        "AArch64TargetMachine.cpp",
        "AArch64TargetObjectFile.cpp",
        "AArch64TargetTransformInfo.cpp",
        "SMEABIPass.cpp",
        "SMEPeepholeOpt.cpp",
        "SVEIntrinsicOpts.cpp",
        "AArch64SIMDInstrOpt.cpp",
    };

    pub const actions = [_]BackendAction{
        .{ .outfile = "AArch64GenAsmMatcher.inc", .td_args = &.{"-gen-asm-matcher"} },
        .{ .outfile = "AArch64GenAsmWriter.inc", .td_args = &.{"-gen-asm-writer"} },
        .{ .outfile = "AArch64GenAsmWriter1.inc", .td_args = &.{"-gen-asm-writer -asmwriternum=1"} },
        .{ .outfile = "AArch64GenCallingConv.inc", .td_args = &.{"-gen-callingconv"} },
        .{ .outfile = "AArch64GenDAGISel.inc", .td_args = &.{"-gen-dag-isel"} },
        .{ .outfile = "AArch64GenDisassemblerTables.inc", .td_args = &.{"-gen-disassembler"} },
        .{ .outfile = "AArch64GenFastISel.inc", .td_args = &.{"-gen-fast-isel"} },
        .{ .outfile = "AArch64GenGlobalISel.inc", .td_args = &.{"-gen-global-isel"} },
        .{
            .outfile = "AArch64GenO0PreLegalizeGICombiner.inc",
            .td_args = &.{ "-gen-global-isel-combiner", "-combiners=AArch64O0PreLegalizerCombiner" },
        },
        .{
            .outfile = "AArch64GenPreLegalizeGICombiner.inc",
            .td_args = &.{ "-gen-global-isel-combiner", "-combiners=AArch64PreLegalizerCombiner" },
        },
        .{
            .outfile = "AArch64GenPostLegalizeGICombiner.inc",
            .td_args = &.{ "-gen-global-isel-combiner", "-combiners=AArch64PostLegalizerCombiner" },
        },
        .{
            .outfile = "AArch64GenPostLegalizeGILowering.inc",
            .td_args = &.{ "-gen-global-isel-combiner", "-combiners=AArch64PostLegalizerLowering" },
        },
        .{ .outfile = "AArch64GenInstrInfo.inc", .td_args = &.{"-gen-instr-info"} },
        .{ .outfile = "AArch64GenMCCodeEmitter.inc", .td_args = &.{"-gen-emitter"} },
        .{ .outfile = "AArch64GenMCPseudoLowering.inc", .td_args = &.{"-gen-pseudo-lowering"} },
        .{ .outfile = "AArch64GenRegisterBank.inc", .td_args = &.{"-gen-register-bank"} },
        .{ .outfile = "AArch64GenRegisterInfo.inc", .td_args = &.{"-gen-register-info"} },
        .{ .outfile = "AArch64GenSDNodeInfo.inc", .td_args = &.{"-gen-sd-node-info"} },
        .{ .outfile = "AArch64GenSubtargetInfo.inc", .td_args = &.{"-gen-subtarget"} },
        .{ .outfile = "AArch64GenSystemOperands.inc", .td_args = &.{"-gen-searchable-tables"} },
        .{ .outfile = "AArch64GenExegesis.inc", .td_args = &.{"-gen-exegesis"} },
    };
};

// X86
// AArch64
// ARM
// RISCV
// WebAssembly
// Xtensa
// PowerPC
// LoongArch