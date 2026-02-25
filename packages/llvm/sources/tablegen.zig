/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TableGen/CMakeLists.txt
pub const lib_root = "llvm/lib/TableGen/";
pub const lib = [_][]const u8{
    "DetailedRecordsBackend.cpp",
    "Error.cpp",
    "JSONBackend.cpp",
    "Main.cpp",
    "Parser.cpp",
    "Record.cpp",
    "SetTheory.cpp",
    "StringMatcher.cpp",
    "StringToOffsetTable.cpp",
    "TableGenBackend.cpp",
    "TableGenBackendSkeleton.cpp",
    "TGLexer.cpp",
    "TGParser.cpp",
    "TGTimer.cpp",
};

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/utils/TableGen/Basic/CMakeLists.txt
pub const basic_root = emitter_root ++ "Basic/";
pub const basic = [_][]const u8{
    "ARMTargetDefEmitter.cpp",
    "Attributes.cpp",
    "CodeGenIntrinsics.cpp",
    "DirectiveEmitter.cpp",
    "IntrinsicEmitter.cpp",
    "RISCVTargetDefEmitter.cpp",
    "RuntimeLibcallsEmitter.cpp",
    "SDNodeProperties.cpp",
    "TableGen.cpp",
    "TargetFeaturesEmitter.cpp",
    "VTEmitter.cpp",
};

pub const minimal_main = emitter_root ++ "llvm-min-tblgen.cpp";

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/utils/TableGen/CMakeLists.txt
pub const common_root = emitter_root ++ "Common/";
pub const common = [_][]const u8{
    "GlobalISel/CodeExpander.cpp",
    "GlobalISel/CombinerUtils.cpp",
    "GlobalISel/CXXPredicates.cpp",
    "GlobalISel/GlobalISelMatchTable.cpp",
    "GlobalISel/GlobalISelMatchTableExecutorEmitter.cpp",
    "GlobalISel/PatternParser.cpp",
    "GlobalISel/Patterns.cpp",

    "AsmWriterInst.cpp",
    "CodeGenDAGPatterns.cpp",
    "CodeGenHwModes.cpp",
    "CodeGenInstAlias.cpp",
    "CodeGenInstruction.cpp",
    "CodeGenRegisters.cpp",
    "CodeGenSchedule.cpp",
    "CodeGenTarget.cpp",
    "DAGISelMatcher.cpp",
    "InfoByHwMode.cpp",
    "OptEmitter.cpp",
    "PredicateExpander.cpp",
    "SubtargetFeatureInfo.cpp",
    "Types.cpp",
    "Utils.cpp",
    "VarLenCodeEmitterGen.cpp",
};

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/utils/TableGen/CMakeLists.txt
pub const emitter_root = "llvm/utils/TableGen/";
pub const emitters = [_][]const u8{
    "AsmMatcherEmitter.cpp",
    "AsmWriterEmitter.cpp",
    "CallingConvEmitter.cpp",
    "CodeEmitterGen.cpp",
    "CodeGenMapTable.cpp",
    "CompressInstEmitter.cpp",
    "CTagsEmitter.cpp",
    "DAGISelEmitter.cpp",
    "DAGISelMatcherEmitter.cpp",
    "DAGISelMatcherGen.cpp",
    "DAGISelMatcherOpt.cpp",
    "DecoderEmitter.cpp",
    "DFAEmitter.cpp",
    "DFAPacketizerEmitter.cpp",
    "DisassemblerEmitter.cpp",
    "DXILEmitter.cpp",
    "ExegesisEmitter.cpp",
    "FastISelEmitter.cpp",
    "GlobalISelCombinerEmitter.cpp",
    "GlobalISelEmitter.cpp",
    "InstrDocsEmitter.cpp",
    "InstrInfoEmitter.cpp",
    "llvm-tblgen.cpp",
    "MacroFusionPredicatorEmitter.cpp",
    "OptionParserEmitter.cpp",
    "OptionRSTEmitter.cpp",
    "PseudoLoweringEmitter.cpp",
    "RegisterBankEmitter.cpp",
    "RegisterInfoEmitter.cpp",
    "SDNodeInfoEmitter.cpp",
    "SearchableTableEmitter.cpp",
    "SubtargetEmitter.cpp",
    "WebAssemblyDisassemblerEmitter.cpp",
    "X86InstrMappingEmitter.cpp",
    "X86DisassemblerTables.cpp",
    "X86FoldTablesEmitter.cpp",
    "X86MnemonicTables.cpp",
    "X86ModRMFilters.cpp",
    "X86RecognizableInstr.cpp",
};
