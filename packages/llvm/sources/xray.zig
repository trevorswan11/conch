//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/XRay/CMakeLists.txt
pub const root = "llvm/lib/XRay";
pub const sources = [_][]const u8{
    "BlockIndexer.cpp",
    "BlockPrinter.cpp",
    "BlockVerifier.cpp",
    "FDRRecordProducer.cpp",
    "FDRRecords.cpp",
    "FDRTraceExpander.cpp",
    "FDRTraceWriter.cpp",
    "FileHeaderReader.cpp",
    "InstrumentationMap.cpp",
    "LogBuilderConsumer.cpp",
    "Profile.cpp",
    "RecordInitializer.cpp",
    "RecordPrinter.cpp",
    "Trace.cpp",
};
