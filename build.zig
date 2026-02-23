const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const flag_opts = addFlagOptions(b);
    const cdb_gen: *CdbGenerator = .init(b);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
        "-Wno-gnu-statement-expression",
        "-Wno-gnu-statement-expression-from-macro-expansion",
        "-DMAGIC_ENUM_RANGE_MAX=255",
    });

    var package_flags = try compiler_flags.clone(b.allocator);
    const dist_flags: []const []const u8 = &.{ "-DNDEBUG", "-DDIST" };
    try package_flags.appendSlice(b.allocator, dist_flags);

    try compiler_flags.appendSlice(b.allocator, &.{
        "-gen-cdb-fragment-path",
        getCacheRelativePath(b, &.{CdbGenerator.cdb_frags_dirname}),
    });
    switch (optimize) {
        .Debug => try compiler_flags.appendSlice(b.allocator, &.{ "-g", "-DDEBUG" }),
        .ReleaseSafe => try compiler_flags.appendSlice(b.allocator, &.{"-DRELEASE"}),
        .ReleaseFast, .ReleaseSmall => try compiler_flags.appendSlice(b.allocator, dist_flags),
    }
    try compiler_flags.append(b.allocator, "-DCATCH_AMALGAMATED_CUSTOM_MAIN");

    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;
    const artifacts = try addArtifacts(b, .{
        .target = b.graph.host,
        .optimize = optimize,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
        .skip_tests = flag_opts.skip_tests,
        .skip_cppcheck = flag_opts.skip_cppcheck,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .cli = artifacts.cli,
        .tests = artifacts.tests,
        .cppcheck = artifacts.cppcheck,
        .clean_cache = flag_opts.clean_cache,
    });

    try addPackageStep(b, .{
        .cxx_flags = package_flags.items,
        .compile_only = flag_opts.compile_only,
    });
}

const ProjectPaths = struct {
    const Project = struct {
        inc: []const u8,
        src: []const u8,
        tests: []const u8,

        pub fn files(self: *const Project, b: *std.Build) ![][]const u8 {
            return std.mem.concat(b.allocator, []const u8, &.{
                try collectFiles(b, self.inc, .{}),
                try collectFiles(b, self.src, .{ .allowed_extensions = &.{".hpp"} }),
                try collectFiles(b, self.tests, .{ .allowed_extensions = &.{ ".hpp", ".cpp" } }),
            });
        }
    };

    const compiler: Project = .{
        .inc = "packages/compiler/include/",
        .src = "packages/compiler/src/",
        .tests = "packages/compiler/tests/",
    };

    const cli: Project = .{
        .inc = "apps/cli/include/",
        .src = "apps/cli/src/",
        .tests = "apps/cli/tests/",
    };

    const core: Project = .{
        .inc = "packages/core/include/",
        .src = "packages/core/src/",
        .tests = "packages/core/tests/",
    };

    const stdlib = "packages/stdlib/";
    const test_runner = "packages/test_runner/";
};

const LibLLVM = struct {
    const Dependencies = struct {
        const Dependency = struct {
            dependency: *std.Build.Dependency,
            artifact: *std.Build.Step.Compile,
        };

        zlib: Dependency,
        libxml2: Dependency,
        zstd: Dependency,
    };

    const version: std.SemanticVersion = .{
        .major = 21,
        .minor = 1,
        .patch = 8,
    };
    const version_str = std.fmt.comptimePrint(
        "{}.{}.{}",
        .{ version.major, version.minor, version.patch },
    );

    /// https://github.com/llvm/llvm-project/blob/main/llvm/utils/TableGen/CMakeLists.txt
    const TableGenSources = struct {
        const basic = [_][]const u8{
            "llvm/lib/TableGen/Error.cpp",
            "llvm/lib/TableGen/Main.cpp",
            "llvm/lib/TableGen/Record.cpp",
            "llvm/lib/TableGen/TGLexer.cpp",
            "llvm/lib/TableGen/TGParser.cpp",
            "llvm/lib/TableGen/TableGenBackend.cpp",
            "llvm/lib/TableGen/DetailedRecordsBackend.cpp",
        };

        const common = [_][]const u8{
            "llvm/utils/TableGen/Common/CodeGenDAGPatterns.cpp",
            "llvm/utils/TableGen/Common/CodeGenHwModes.cpp",
            "llvm/utils/TableGen/Common/CodeGenInstruction.cpp",
            "llvm/utils/TableGen/Common/CodeGenRegisters.cpp",
            "llvm/utils/TableGen/Common/CodeGenSchedule.cpp",
            "llvm/utils/TableGen/Common/CodeGenTarget.cpp",
            "llvm/utils/TableGen/Common/DAGISelMatcher.cpp",
            "llvm/utils/TableGen/Common/GlobalISel/CXXPredicates.cpp",
            "llvm/utils/TableGen/Common/GlobalISel/Patterns.cpp",
            "llvm/utils/TableGen/Common/InfoByHwMode.cpp",
            "llvm/utils/TableGen/Common/OptEmitter.cpp",
            "llvm/utils/TableGen/Common/PredicateExpander.cpp",
            "llvm/utils/TableGen/Common/SubtargetFeatureInfo.cpp",
            "llvm/utils/TableGen/Common/VarLenCodeEmitterGen.cpp",
        };

        const emitters = [_][]const u8{
            "llvm/utils/TableGen/AsmMatcherEmitter.cpp",
            "llvm/utils/TableGen/AsmWriterEmitter.cpp",
            "llvm/utils/TableGen/CallingConvEmitter.cpp",
            "llvm/utils/TableGen/CodeEmitterGen.cpp",
            "llvm/utils/TableGen/CodeGenMapTable.cpp",
            "llvm/utils/TableGen/CompressInstEmitter.cpp",
            "llvm/utils/TableGen/CTagsEmitter.cpp",
            "llvm/utils/TableGen/DAGISelEmitter.cpp",
            "llvm/utils/TableGen/DecoderEmitter.cpp",
            "llvm/utils/TableGen/DecoderTableEmitter.cpp",
            "llvm/utils/TableGen/DecoderTree.cpp",
            "llvm/utils/TableGen/DFAEmitter.cpp",
            "llvm/utils/TableGen/DFAPacketizerEmitter.cpp",
            "llvm/utils/TableGen/DisassemblerEmitter.cpp",
            "llvm/utils/TableGen/DXILEmitter.cpp",
            "llvm/utils/TableGen/ExegesisEmitter.cpp",
            "llvm/utils/TableGen/FastISelEmitter.cpp",
            "llvm/utils/TableGen/GlobalISelCombinerEmitter.cpp",
            "llvm/utils/TableGen/GlobalISelEmitter.cpp",
            "llvm/utils/TableGen/InstrDocsEmitter.cpp",
            "llvm/utils/TableGen/InstrInfoEmitter.cpp",
            "llvm/utils/TableGen/llvm-tblgen.cpp",
            "llvm/utils/TableGen/MacroFusionPredicatorEmitter.cpp",
            "llvm/utils/TableGen/OptionParserEmitter.cpp",
            "llvm/utils/TableGen/OptionRSTEmitter.cpp",
            "llvm/utils/TableGen/PseudoLoweringEmitter.cpp",
            "llvm/utils/TableGen/RegisterBankEmitter.cpp",
            "llvm/utils/TableGen/RegisterInfoEmitter.cpp",
            "llvm/utils/TableGen/SDNodeInfoEmitter.cpp",
            "llvm/utils/TableGen/SearchableTableEmitter.cpp",
            "llvm/utils/TableGen/SubtargetEmitter.cpp",
            "llvm/utils/TableGen/WebAssemblyDisassemblerEmitter.cpp",
            // Backend specific (X86)
            "llvm/utils/TableGen/X86InstrMappingEmitter.cpp",
            "llvm/utils/TableGen/X86DisassemblerTables.cpp",
            "llvm/utils/TableGen/X86FoldTablesEmitter.cpp",
            "llvm/utils/TableGen/X86MnemonicTables.cpp",
            "llvm/utils/TableGen/X86ModRMFilters.cpp",
            "llvm/utils/TableGen/X86RecognizableInstr.cpp",
        };
    };

    /// https://github.com/llvm/llvm-project/blob/main/llvm/lib/Support/CMakeLists.txt
    const SupportSources = struct {
        const common = [_][]const u8{
            "llvm/lib/Support/ABIBreak.cpp",
            "llvm/lib/Support/AMDGPUMetadata.cpp",
            "llvm/lib/Support/APFixedPoint.cpp",
            "llvm/lib/Support/APFloat.cpp",
            "llvm/lib/Support/APInt.cpp",
            "llvm/lib/Support/APSInt.cpp",
            "llvm/lib/Support/ARMBuildAttributes.cpp",
            "llvm/lib/Support/AArch64AttributeParser.cpp",
            "llvm/lib/Support/AArch64BuildAttributes.cpp",
            "llvm/lib/Support/ARMAttributeParser.cpp",
            "llvm/lib/Support/ARMWinEH.cpp",
            "llvm/lib/Support/Allocator.cpp",
            "llvm/lib/Support/AutoConvert.cpp",
            "llvm/lib/Support/Base64.cpp",
            "llvm/lib/Support/BalancedPartitioning.cpp",
            "llvm/lib/Support/BinaryStreamError.cpp",
            "llvm/lib/Support/BinaryStreamReader.cpp",
            "llvm/lib/Support/BinaryStreamRef.cpp",
            "llvm/lib/Support/BinaryStreamWriter.cpp",
            "llvm/lib/Support/BlockFrequency.cpp",
            "llvm/lib/Support/BranchProbability.cpp",
            "llvm/lib/Support/BuryPointer.cpp",
            "llvm/lib/Support/CachePruning.cpp",
            "llvm/lib/Support/Caching.cpp",
            "llvm/lib/Support/circular_raw_ostream.cpp",
            "llvm/lib/Support/Chrono.cpp",
            "llvm/lib/Support/COM.cpp",
            "llvm/lib/Support/CodeGenCoverage.cpp",
            "llvm/lib/Support/CommandLine.cpp",
            "llvm/lib/Support/Compression.cpp",
            "llvm/lib/Support/CRC.cpp",
            "llvm/lib/Support/ConvertUTF.cpp",
            "llvm/lib/Support/ConvertEBCDIC.cpp",
            "llvm/lib/Support/ConvertUTFWrapper.cpp",
            "llvm/lib/Support/CrashRecoveryContext.cpp",
            "llvm/lib/Support/CSKYAttributes.cpp",
            "llvm/lib/Support/CSKYAttributeParser.cpp",
            "llvm/lib/Support/DataExtractor.cpp",
            "llvm/lib/Support/Debug.cpp",
            "llvm/lib/Support/DebugCounter.cpp",
            "llvm/lib/Support/DeltaAlgorithm.cpp",
            "llvm/lib/Support/DeltaTree.cpp",
            "llvm/lib/Support/DivisionByConstantInfo.cpp",
            "llvm/lib/Support/DAGDeltaAlgorithm.cpp",
            "llvm/lib/Support/DJB.cpp",
            "llvm/lib/Support/DynamicAPInt.cpp",
            "llvm/lib/Support/ELFAttributes.cpp",
            "llvm/lib/Support/ELFAttrParserCompact.cpp",
            "llvm/lib/Support/ELFAttrParserExtended.cpp",
            "llvm/lib/Support/Error.cpp",
            "llvm/lib/Support/ErrorHandling.cpp",
            "llvm/lib/Support/ExponentialBackoff.cpp",
            "llvm/lib/Support/ExtensibleRTTI.cpp",
            "llvm/lib/Support/FileCollector.cpp",
            "llvm/lib/Support/FileUtilities.cpp",
            "llvm/lib/Support/FileOutputBuffer.cpp",
            "llvm/lib/Support/FloatingPointMode.cpp",
            "llvm/lib/Support/FoldingSet.cpp",
            "llvm/lib/Support/FormattedStream.cpp",
            "llvm/lib/Support/FormatVariadic.cpp",
            "llvm/lib/Support/GlobPattern.cpp",
            "llvm/lib/Support/GraphWriter.cpp",
            "llvm/lib/Support/HexagonAttributeParser.cpp",
            "llvm/lib/Support/HexagonAttributes.cpp",
            "llvm/lib/Support/InitLLVM.cpp",
            "llvm/lib/Support/InstructionCost.cpp",
            "llvm/lib/Support/IntEqClasses.cpp",
            "llvm/lib/Support/IntervalMap.cpp",
            "llvm/lib/Support/JSON.cpp",
            "llvm/lib/Support/KnownBits.cpp",
            "llvm/lib/Support/KnownFPClass.cpp",
            "llvm/lib/Support/LEB128.cpp",
            "llvm/lib/Support/LineIterator.cpp",
            "llvm/lib/Support/Locale.cpp",
            "llvm/lib/Support/LockFileManager.cpp",
            "llvm/lib/Support/ManagedStatic.cpp",
            "llvm/lib/Support/MathExtras.cpp",
            "llvm/lib/Support/MemAlloc.cpp",
            "llvm/lib/Support/MemoryBuffer.cpp",
            "llvm/lib/Support/MemoryBufferRef.cpp",
            "llvm/lib/Support/ModRef.cpp",
            "llvm/lib/Support/MD5.cpp",
            "llvm/lib/Support/MSP430Attributes.cpp",
            "llvm/lib/Support/MSP430AttributeParser.cpp",
            "llvm/lib/Support/Mustache.cpp",
            "llvm/lib/Support/NativeFormatting.cpp",
            "llvm/lib/Support/OptimizedStructLayout.cpp",
            "llvm/lib/Support/Optional.cpp",
            "llvm/lib/Support/OptionStrCmp.cpp",
            "llvm/lib/Support/PGOOptions.cpp",
            "llvm/lib/Support/Parallel.cpp",
            "llvm/lib/Support/PluginLoader.cpp",
            "llvm/lib/Support/PrettyStackTrace.cpp",
            "llvm/lib/Support/RandomNumberGenerator.cpp",
            "llvm/lib/Support/Regex.cpp",
            "llvm/lib/Support/RewriteBuffer.cpp",
            "llvm/lib/Support/RewriteRope.cpp",
            "llvm/lib/Support/RISCVAttributes.cpp",
            "llvm/lib/Support/RISCVAttributeParser.cpp",
            "llvm/lib/Support/RISCVISAUtils.cpp",
            "llvm/lib/Support/ScaledNumber.cpp",
            "llvm/lib/Support/ScopedPrinter.cpp",
            "llvm/lib/Support/SHA1.cpp",
            "llvm/lib/Support/SHA256.cpp",
            "llvm/lib/Support/Signposts.cpp",
            "llvm/lib/Support/SipHash.cpp",
            "llvm/lib/Support/SlowDynamicAPInt.cpp",
            "llvm/lib/Support/SmallPtrSet.cpp",
            "llvm/lib/Support/SmallVector.cpp",
            "llvm/lib/Support/SourceMgr.cpp",
            "llvm/lib/Support/SpecialCaseList.cpp",
            "llvm/lib/Support/Statistic.cpp",
            "llvm/lib/Support/StringExtras.cpp",
            "llvm/lib/Support/StringMap.cpp",
            "llvm/lib/Support/StringSaver.cpp",
            "llvm/lib/Support/StringRef.cpp",
            "llvm/lib/Support/SuffixTreeNode.cpp",
            "llvm/lib/Support/SuffixTree.cpp",
            "llvm/lib/Support/SystemUtils.cpp",
            "llvm/lib/Support/TarWriter.cpp",
            "llvm/lib/Support/TextEncoding.cpp",
            "llvm/lib/Support/ThreadPool.cpp",
            "llvm/lib/Support/TimeProfiler.cpp",
            "llvm/lib/Support/Timer.cpp",
            "llvm/lib/Support/ToolOutputFile.cpp",
            "llvm/lib/Support/TrieRawHashMap.cpp",
            "llvm/lib/Support/Twine.cpp",
            "llvm/lib/Support/Unicode.cpp",
            "llvm/lib/Support/UnicodeCaseFold.cpp",
            "llvm/lib/Support/UnicodeNameToCodepoint.cpp",
            "llvm/lib/Support/UnicodeNameToCodepointGenerated.cpp",
            "llvm/lib/Support/VersionTuple.cpp",
            "llvm/lib/Support/VirtualFileSystem.cpp",
            "llvm/lib/Support/WithColor.cpp",
            "llvm/lib/Support/YAMLParser.cpp",
            "llvm/lib/Support/YAMLTraits.cpp",
            "llvm/lib/Support/raw_os_ostream.cpp",
            "llvm/lib/Support/raw_ostream.cpp",
            "llvm/lib/Support/raw_socket_stream.cpp",
            "llvm/lib/Support/xxhash.cpp",
            "llvm/lib/Support/Z3Solver.cpp",
            "llvm/lib/Support/Atomic.cpp",
            "llvm/lib/Support/DynamicLibrary.cpp",
            "llvm/lib/Support/Errno.cpp",
            "llvm/lib/Support/Memory.cpp",
            "llvm/lib/Support/Path.cpp",
            "llvm/lib/Support/Process.cpp",
            "llvm/lib/Support/Program.cpp",
            "llvm/lib/Support/Signals.cpp",
            "llvm/lib/Support/Threading.cpp",
            "llvm/lib/Support/Valgrind.cpp",
            "llvm/lib/Support/Watchdog.cpp",
        };

        const regex_c = [_][]const u8{
            "llvm/lib/Support/regcomp.c",
            "llvm/lib/Support/regerror.c",
            "llvm/lib/Support/regexec.c",
            "llvm/lib/Support/regfree.c",
            "llvm/lib/Support/regstrlcpy.c",
        };

        const blake3 = [_][]const u8{
            "llvm/lib/Support/BLAKE3/blake3.c",
            "llvm/lib/Support/BLAKE3/blake3_dispatch.c",
            "llvm/lib/Support/BLAKE3/blake3_portable.c",
        };

        const windows_specific = [_][]const u8{
            "llvm/lib/Support/Windows/DynamicLibrary.inc",
            "llvm/lib/Support/Windows/Memory.inc",
            "llvm/lib/Support/Windows/Path.inc",
            "llvm/lib/Support/Windows/Process.inc",
            "llvm/lib/Support/Windows/Program.inc",
            "llvm/lib/Support/Windows/Signals.inc",
            "llvm/lib/Support/Windows/Threading.inc",
            "llvm/lib/Support/Windows/Watchdog.inc",
        };
    };

    b: *std.Build,
    llvm_dep: *std.Build.Dependency,
    llvm_root: std.Build.LazyPath,

    host_tablegen: *std.Build.Step.Compile = undefined,
    host_deps: Dependencies = undefined,
    host_support: *std.Build.Step.Compile = undefined,
    target_deps: Dependencies = undefined,
    target_support: *std.Build.Step.Compile = undefined,

    pub fn build(b: *std.Build, config: struct {
        target: std.Build.ResolvedTarget,
        auto_install: bool,
    }) !LibLLVM {
        const upstream = b.dependency("llvm", .{});
        var llvm: LibLLVM = .{
            .b = b,
            .llvm_dep = upstream,
            .llvm_root = upstream.path("."),
        };

        // Host Dependencies for TableGen
        llvm.host_deps = try llvm.dependencies(.{
            .target = b.graph.host,
            .auto_install = false,
        });

        llvm.host_support = try llvm.support(.{
            .target = b.graph.host,
            .optimize = .ReleaseSafe,
            .deps = llvm.host_deps,
        });
        llvm.host_tablegen = llvm.tablegen(llvm.host_support);

        // Target dependencies for conch
        llvm.target_deps = try llvm.dependencies(.{
            .target = config.target,
            .auto_install = config.auto_install,
        });
        llvm.target_support = try llvm.support(.{
            .target = config.target,
            .optimize = .ReleaseSafe,
            .deps = llvm.target_deps,
        });

        return llvm;
    }

    /// Need More, see https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/Config/Targets.h.cmake
    /// and friends
    fn createConfigHeaders(
        self: *const LibLLVM,
        target: std.Build.ResolvedTarget,
    ) !struct {
        internal_config: *std.Build.Step.ConfigHeader,
        external_config: *std.Build.Step.ConfigHeader,
    } {
        const b = self.b;
        const t = target.result;
        const is_darwin = t.os.tag.isDarwin();
        const is_windows = t.os.tag == .windows;
        const is_linux = t.os.tag == .linux;

        const native_arch = switch (t.cpu.arch) {
            .x86_64 => "X86",
            .aarch64 => "AArch64",
            .riscv64 => "RISCV",
            else => "X86",
        };
        const triple = try t.linuxTriple(b.allocator);

        // https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/Config/config.h.cmake
        const internal = b.addConfigHeader(.{
            .style = .{ .cmake = self.llvm_root.path(b, "llvm/include/llvm/Config/config.h.cmake") },
            .include_path = "llvm/Config/config.h",
        }, .{
            .PACKAGE_NAME = "LLVM",
            .PACKAGE_VERSION = version_str,
            .PACKAGE_STRING = "LLVM-" ++ version_str,
            .PACKAGE_BUGREPORT = "https://github.com/llvm/llvm-project/issues/",
            .PACKAGE_VENDOR = "Conch Project",
            .BUG_REPORT_URL = "https://github.com/llvm/llvm-project/issues/",

            .ENABLE_BACKTRACES = 1,
            .ENABLE_CRASH_OVERRIDES = 1,
            .LLVM_ENABLE_CRASH_DUMPS = 0,
            .LLVM_WINDOWS_PREFER_FORWARD_SLASH = @intFromBool(is_windows),

            // Functions and Headers
            .HAVE_UNISTD_H = @intFromBool(!is_windows),
            .HAVE_SYS_MMAN_H = @intFromBool(!is_windows),
            .HAVE_SYS_IOCTL_H = @intFromBool(!is_windows),
            .HAVE_POSIX_SPAWN = @intFromBool(!is_windows),
            .HAVE_PREAD = @intFromBool(!is_windows),
            .HAVE_PTHREAD_H = @intFromBool(!is_windows),
            .HAVE_FUTIMENS = @intFromBool(!is_windows),
            .HAVE_FUTIMES = @intFromBool(!is_windows),
            .HAVE_GETPAGESIZE = @intFromBool(!is_windows),
            .HAVE_GETRUSAGE = @intFromBool(!is_windows),
            .HAVE_ISATTY = 1,
            .HAVE_SETENV = @intFromBool(!is_windows),
            .HAVE_STRERROR_R = @intFromBool(!is_windows),
            .HAVE_SYSCONF = @intFromBool(!is_windows),
            .HAVE_SIGALTSTACK = @intFromBool(!is_windows),
            .HAVE_DLOPEN = @intFromBool(!is_windows),
            .HAVE_BACKTRACE = @intFromBool(!is_windows),
            .HAVE_SBRK = @intFromBool(!is_windows),

            // Darwin Specific
            .HAVE_MACH_MACH_H = @intFromBool(is_darwin),
            .HAVE_MALLOC_MALLOC_H = @intFromBool(is_darwin),
            .HAVE_MALLOC_ZONE_STATISTICS = @intFromBool(is_darwin),
            .HAVE_PROC_PID_RUSAGE = @intFromBool(is_darwin),
            .HAVE_CRASHREPORTER_INFO = @intFromBool(is_darwin),
            .HAVE_CRASHREPORTERCLIENT_H = 0,

            // Windows Specific
            .HAVE_LIBPSAPI = @intFromBool(is_windows),
            .HAVE__CHSIZE_S = @intFromBool(is_windows),
            .HAVE__ALLOCA = @intFromBool(is_windows),
            .stricmp = if (is_windows) "_stricmp" else "stricmp",
            .strdup = if (is_windows) "_strdup" else "strdup",

            // Allocation & Threading
            .HAVE_MALLINFO = @intFromBool(is_linux),
            .HAVE_MALLINFO2 = @intFromBool(is_linux),
            .HAVE_MALLCTL = 0,
            .HAVE_PTHREAD_GETNAME_NP = @intFromBool(is_linux or is_darwin),
            .HAVE_PTHREAD_SETNAME_NP = @intFromBool(is_linux or is_darwin),
            .HAVE_PTHREAD_GET_NAME_NP = 0,
            .HAVE_PTHREAD_SET_NAME_NP = 0,
            .HAVE_PTHREAD_MUTEX_LOCK = 1,
            .HAVE_PTHREAD_RWLOCK_INIT = 1,
            .HAVE_LIBPTHREAD = @intFromBool(!is_windows),

            // GlobalISel & Extras
            .LLVM_GISEL_COV_ENABLED = 0,
            .LLVM_GISEL_COV_PREFIX = "",
            .LLVM_ENABLE_LIBXML2 = 1,
            .HAVE_ICU = 0,
            .HAVE_ICONV = 0,
            .LLVM_SUPPORT_XCODE_SIGNPOSTS = @intFromBool(is_darwin),
            .BACKTRACE_HEADER = if (is_darwin) "execinfo.h" else "link.h",
            .HOST_LINK_VERSION = "0",
            .LLVM_TARGET_TRIPLE_ENV = "",
            .LLVM_VERSION_PRINTER_SHOW_HOST_TARGET_INFO = 1,
            .LLVM_VERSION_PRINTER_SHOW_BUILD_CONFIG = 1,

            // Missing Boilerplate for LLVM headers
            .HAVE_FFI_CALL = 0,
            .HAVE_FFI_FFI_H = 0,
            .HAVE_FFI_H = 0,
            .HAVE_LIBEDIT = 0,
            .HAVE_LIBPFM = 0,
            .LIBPFM_HAS_FIELD_CYCLES = 0,
            .HAVE_REGISTER_FRAME = 0,
            .HAVE_DEREGISTER_FRAME = 0,
            .HAVE_UNW_ADD_DYNAMIC_FDE = 0,
            .HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC = @intFromBool(is_darwin),
            .HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC = @intFromBool(is_linux),
            .HAVE_VALGRIND_VALGRIND_H = 0,
            .HAVE__UNWIND_BACKTRACE = 0,
            .HAVE_DECL_ARC4RANDOM = @intFromBool(is_darwin),
            .HAVE_DECL_FE_ALL_EXCEPT = 1,
            .HAVE_DECL_FE_INEXACT = 1,
            .HAVE_DECL_STRERROR_S = @intFromBool(is_windows),

            // Host Intrinsics (Safe to set 0 for most modern compilers via Zig)
            .HAVE___ALLOCA = 0,
            .HAVE___ASHLDI3 = 0,
            .HAVE___ASHRDI3 = 0,
            .HAVE___CHKSTK = 0,
            .HAVE___CHKSTK_MS = 0,
            .HAVE___CMPDI2 = 0,
            .HAVE___DIVDI3 = 0,
            .HAVE___FIXDFDI = 0,
            .HAVE___FIXSFDI = 0,
            .HAVE___FLOATDIDF = 0,
            .HAVE___LSHRDI3 = 0,
            .HAVE___MAIN = 0,
            .HAVE___MODDI3 = 0,
            .HAVE___UDIVDI3 = 0,
            .HAVE___UMODDI3 = 0,
            .HAVE____CHKSTK = 0,
            .HAVE____CHKSTK_MS = 0,

            // Compiler Intrinsics
            .HAVE_BUILTIN_THREAD_POINTER = 1,
            .HAVE_GETAUXVAL = @intFromBool(is_linux),

            // Extensions
            .LTDL_SHLIB_EXT = t.dynamicLibSuffix(),
            .LLVM_PLUGIN_EXT = t.dynamicLibSuffix(),
        });

        // https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/Config/llvm-config.h.cmake
        const external = b.addConfigHeader(.{
            .style = .{ .cmake = self.llvm_root.path(b, "llvm/include/llvm/Config/llvm-config.h.cmake") },
            .include_path = "llvm/Config/llvm-config.h",
        }, .{
            .LLVM_VERSION_MAJOR = @as(i64, @intCast(version.major)),
            .LLVM_VERSION_MINOR = @as(i64, @intCast(version.minor)),
            .LLVM_VERSION_PATCH = @as(i64, @intCast(version.patch)),
            .PACKAGE_VERSION = version_str,

            .LLVM_DEFAULT_TARGET_TRIPLE = triple,
            .LLVM_HOST_TRIPLE = triple,

            .LLVM_ENABLE_THREADS = 1,
            .LLVM_HAS_ATOMICS = 1,
            .LLVM_ON_UNIX = @intFromBool(!is_windows),
            .LLVM_ENABLE_LLVM_C_EXPORT_ANNOTATIONS = 0,
            .LLVM_ENABLE_LLVM_EXPORT_ANNOTATIONS = 0,

            // Native Target Bootstrapping
            .LLVM_NATIVE_ARCH = native_arch,
            .LLVM_ENABLE_ZLIB = 1,
            .LLVM_ENABLE_ZSTD = 1,
            .LLVM_ENABLE_CURL = 0,
            .LLVM_ENABLE_HTTPLIB = 0,
            .LLVM_WITH_Z3 = 0,

            // Optimization & Debug
            .LLVM_UNREACHABLE_OPTIMIZE = 1,
            .LLVM_ENABLE_DUMP = 1,
            .LLVM_ENABLE_DIA_SDK = 0,
            .HAVE_SYSEXITS_H = @intFromBool(!is_windows),

            // Missing Logic for Native Initialization
            .LLVM_NATIVE_ASMPARSER = null,
            .LLVM_NATIVE_ASMPRINTER = null,
            .LLVM_NATIVE_DISASSEMBLER = null,
            .LLVM_NATIVE_TARGET = null,
            .LLVM_NATIVE_TARGETINFO = null,
            .LLVM_NATIVE_TARGETMC = null,
            .LLVM_NATIVE_TARGETMCA = null,

            // Performance & Stats
            .LLVM_USE_INTEL_JITEVENTS = 0,
            .LLVM_USE_OPROFILE = 0,
            .LLVM_USE_PERF = 0,
            .LLVM_FORCE_ENABLE_STATS = 0,
            .LLVM_ENABLE_PROFCHECK = 0,
            .LLVM_ENABLE_TELEMETRY = 0,

            // Feature Flags
            .LLVM_HAVE_TFLITE = 0,
            .LLVM_BUILD_LLVM_DYLIB = 0,
            .LLVM_BUILD_SHARED_LIBS = 0,
            .LLVM_FORCE_USE_OLD_TOOLCHAIN = 0,
            .LLVM_ENABLE_IO_SANDBOX = 0,
            .LLVM_ENABLE_PLUGINS = 0,
            .LLVM_HAS_LOGF128 = 0,

            // Debug Location Tracking
            .LLVM_ENABLE_DEBUGLOC_TRACKING_COVERAGE = 0,
            .LLVM_ENABLE_DEBUGLOC_TRACKING_ORIGIN = 0,

            // Storage
            .LLVM_ENABLE_ONDISK_CAS = 0,
        });

        return .{
            .internal_config = internal,
            .external_config = external,
        };
    }

    fn support(self: *const LibLLVM, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
        deps: Dependencies,
    }) !*std.Build.Step.Compile {
        const b = self.b;
        const t = config.target.result;

        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
            .link_libcpp = true,
        });

        // Standard LLVM Defines for Windows
        if (t.os.tag == .windows) {
            mod.addCMacro("_CRT_SECURE_NO_DEPRECATE", "");
            mod.addCMacro("_CRT_SECURE_NO_WARNINGS", "");
            mod.addCMacro("_CRT_NONSTDC_NO_DEPRECATE", "");
            mod.addCMacro("_CRT_NONSTDC_NO_WARNINGS", "");
            mod.addCMacro("_SCL_SECURE_NO_WARNINGS", "");
            mod.addCMacro("UNICODE", "");
            mod.addCMacro("_UNICODE", "");
        }

        const configs = try self.createConfigHeaders(config.target);
        mod.addIncludePath(configs.internal_config.getOutputDir());
        mod.addIncludePath(configs.external_config.getOutputDir());
        mod.addIncludePath(self.llvm_root.path(b, "llvm/include"));
        mod.addIncludePath(self.llvm_root.path(b, "llvm/lib/Support"));

        // Compile sources
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &SupportSources.regex_c,
            .flags = &.{"-std=c11"},
        });
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &SupportSources.blake3,
            .flags = &.{"-std=c11"},
        });

        const cpp_flags = [_][]const u8{ "-std=c++17", "-fno-exceptions", "-fno-rtti" };
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &SupportSources.common,
            .flags = &cpp_flags,
        });
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &SupportSources.common,
            .flags = &cpp_flags,
        });

        mod.linkLibrary(config.deps.zlib.artifact);
        mod.linkLibrary(config.deps.zstd.artifact);
        mod.linkLibrary(config.deps.libxml2.artifact);

        // Windows System Libraries
        if (t.os.tag == .windows) {
            mod.linkSystemLibrary("psapi", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("shell32", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("ole32", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("uuid", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("advapi32", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("ws2_32", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("ntdll", .{ .preferred_link_mode = .static });
        } else {
            mod.linkSystemLibrary("m", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("pthread", .{ .preferred_link_mode = .static });
        }

        return b.addLibrary(.{
            .name = b.fmt("LLVMSupport_{s}", .{@tagName(t.cpu.arch)}),
            .root_module = mod,
        });
    }

    fn tablegen(
        self: *const LibLLVM,
        support_lib: *std.Build.Step.Compile,
    ) *std.Build.Step.Compile {
        const b = self.b;
        const mod = b.createModule(.{
            .target = b.graph.host,
            .optimize = .ReleaseSafe,
            .link_libc = true,
        });

        const cpp_flags = [_][]const u8{
            "-std=c++17",
            "-fno-exceptions",
            "-fno-rtti",
        };

        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &TableGenSources.emitters,
            .flags = &cpp_flags,
        });
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &TableGenSources.basic,
            .flags = &cpp_flags,
        });
        mod.addCSourceFiles(.{
            .root = self.llvm_root,
            .files = &TableGenSources.common,
            .flags = &cpp_flags,
        });

        mod.addIncludePath(self.llvm_root.path(b, "llvm/include"));
        mod.addIncludePath(self.llvm_root.path(b, "llvm/utils/TableGen"));

        const exe = b.addExecutable(.{
            .name = "llvm-tblgen",
            .root_module = mod,
        });
        exe.linkLibrary(support_lib);

        return exe;
    }

    pub fn dependencies(self: *const LibLLVM, config: struct {
        target: std.Build.ResolvedTarget,
        auto_install: bool,
    }) !Dependencies {
        const optimize: std.builtin.OptimizeMode = .ReleaseSafe;
        const zlib_dep = try self.zlib(.{
            .target = config.target,
            .optimize = optimize,
        });
        const zlib_include = zlib_dep.dependency.path(".");

        const libxml2_dep = self.libxml2(.{
            .target = config.target,
            .optimize = optimize,
            .zlib_include = zlib_include,
        });

        const zstd_dep = self.zstd(.{
            .target = config.target,
            .optimize = optimize,
            .zlib_include = zlib_include,
        });

        const b = self.b;
        if (config.auto_install) {
            b.installArtifact(zlib_dep.artifact);
            b.installArtifact(libxml2_dep.artifact);
            b.installArtifact(zstd_dep.artifact);
        }

        return .{
            .zlib = zlib_dep,
            .libxml2 = libxml2_dep,
            .zstd = zstd_dep,
        };
    }

    /// Compiles zlib from source as a static library
    /// Reference: https://github.com/allyourcodebase/zlib
    fn zlib(self: *const LibLLVM, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
    }) !Dependencies.Dependency {
        const b = self.b;
        const upstream = b.dependency("zlib", .{});
        const root = upstream.path(".");
        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
        });

        const src_files = [_][]const u8{
            "adler32.c", "compress.c", "crc32.c",   "deflate.c",
            "gzclose.c", "gzlib.c",    "gzread.c",  "gzwrite.c",
            "infback.c", "inffast.c",  "inflate.c", "inftrees.c",
            "trees.c",   "uncompr.c",  "zutil.c",
        };

        var flags: std.ArrayList([]const u8) = .empty;
        try flags.appendSlice(b.allocator, &.{ "-std=c11", "-D_REENTRANT" });
        if (config.target.result.os.tag != .windows) {
            try flags.appendSlice(b.allocator, &.{ "-DHAVE_UNISTD_H", "-DHAVE_SYS_TYPES_H" });
        } else {
            try flags.append(b.allocator, "-DWIN32");
        }

        mod.addCSourceFiles(.{
            .root = root,
            .files = &src_files,
            .flags = flags.items,
        });
        mod.addIncludePath(root);

        const lib = b.addLibrary(.{
            .name = "z",
            .root_module = mod,
        });
        lib.installHeadersDirectory(upstream.path(""), "zlib", .{
            .include_extensions = &.{
                "zconf.h",
                "zlib.h",
            },
        });

        return .{
            .dependency = upstream,
            .artifact = lib,
        };
    }

    /// Compiles libxml2 from source as a static library
    /// Reference: https://github.com/allyourcodebase/libxml2
    fn libxml2(self: *const LibLLVM, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
        zlib_include: std.Build.LazyPath,
    }) Dependencies.Dependency {
        const b = self.b;
        const upstream = b.dependency("libxml2", .{});
        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
        });

        // CMake generates this required file usually
        const config_header = b.addConfigHeader(.{
            .style = .{
                .cmake = upstream.path("config.h.cmake.in"),
            },
            .include_path = "config.h",
        }, .{
            .HAVE_STDLIB_H = 1,
            .HAVE_STDINT_H = 1,
            .HAVE_STAT = 1,
            .HAVE_FSTAT = 1,
            .HAVE_FUNC_ATTRIBUTE_DESTRUCTOR = 1,
            .HAVE_LIBHISTORY = 0,
            .HAVE_LIBREADLINE = 0,
            .XML_SYSCONFDIR = 0,

            // Platform-specific logic
            .HAVE_DLOPEN = @intFromBool(config.target.result.os.tag != .windows),
            .XML_THREAD_LOCAL = switch (config.target.result.os.tag) {
                .windows => "__declspec(thread)",
                else => "_Thread_local",
            },
        });
        mod.addConfigHeader(config_header);

        // Autotools generates this required file usually
        const xmlversion_header = b.addConfigHeader(.{
            .style = .{
                .autoconf_at = upstream.path("include/libxml/xmlversion.h.in"),
            },
            .include_path = "libxml/xmlversion.h",
        }, .{
            .VERSION = "2.15.1",
            .LIBXML_VERSION_NUMBER = 21501,
            .LIBXML_VERSION_EXTRA = "-conch-static",
            .WITH_THREADS = 1,
            .WITH_THREAD_ALLOC = 1,
            .WITH_OUTPUT = 1,
            .WITH_PUSH = 1,
            .WITH_READER = 1,
            .WITH_PATTERN = 1,
            .WITH_WRITER = 1,
            .WITH_SAX1 = 1,
            .WITH_HTTP = 0,
            .WITH_VALID = 1,
            .WITH_HTML = 1,
            .WITH_C14N = 0,
            .WITH_CATALOG = 0,
            .WITH_XPATH = 1,
            .WITH_XPTR = 1,
            .WITH_XINCLUDE = 1,
            .WITH_ICONV = 1,
            .WITH_ICU = 0,
            .WITH_ISO8859X = 1,
            .WITH_DEBUG = 1,
            .WITH_REGEXPS = 1,
            .WITH_RELAXNG = 0,
            .WITH_SCHEMAS = 0,
            .WITH_SCHEMATRON = 0,
            .WITH_MODULES = 0,
            .WITH_ZLIB = 1,
            .MODULE_EXTENSION = config.target.result.dynamicLibSuffix(),
        });
        mod.addConfigHeader(xmlversion_header);

        mod.addCSourceFiles(.{
            .root = upstream.path("."),
            .files = &.{
                "buf.c",       "dict.c",      "entities.c", "error.c",           "globals.c",
                "hash.c",      "list.c",      "parser.c",   "parserInternals.c", "SAX2.c",
                "threads.c",   "tree.c",      "uri.c",      "valid.c",           "xmlIO.c",
                "xmlmemory.c", "xmlstring.c",
            },
            .flags = &.{ "-std=c11", "-D_REENTRANT" },
        });
        mod.addIncludePath(upstream.path("include"));
        mod.addIncludePath(config.zlib_include);

        const lib = b.addLibrary(.{
            .name = "xml2",
            .root_module = mod,
        });
        lib.installConfigHeader(xmlversion_header);
        lib.installHeadersDirectory(upstream.path("include/libxml"), "libxml", .{});

        return .{
            .dependency = upstream,
            .artifact = lib,
        };
    }

    /// Compiles libxml2 from source as a static library
    /// Reference: https://github.com/allyourcodebase/zstd
    fn zstd(self: *const LibLLVM, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
        zlib_include: std.Build.LazyPath,
    }) Dependencies.Dependency {
        const b = self.b;
        const upstream = b.dependency("zstd", .{});
        const lib_path = upstream.path("lib");

        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
        });

        // Common
        mod.addCSourceFiles(.{
            .root = lib_path,
            .files = &.{
                "common/zstd_common.c",    "common/threading.c", "common/entropy_common.c",
                "common/fse_decompress.c", "common/xxhash.c",    "common/error_private.c",
                "common/pool.c",
            },
            .flags = &.{ "-std=c99", "-DZSTD_MULTITHREAD=1" },
        });

        // Compression & Decompression
        mod.addCSourceFiles(.{
            .root = lib_path,
            .files = &.{
                "compress/fse_compress.c",            "compress/huf_compress.c",      "compress/zstd_double_fast.c",
                "compress/zstd_compress_literals.c",  "compress/zstdmt_compress.c",   "compress/zstd_compress_superblock.c",
                "compress/zstd_opt.c",                "compress/zstd_compress.c",     "compress/zstd_compress_sequences.c",
                "compress/hist.c",                    "compress/zstd_ldm.c",          "compress/zstd_lazy.c",
                "compress/zstd_fast.c",               "decompress/zstd_decompress.c", "decompress/huf_decompress.c",
                "decompress/zstd_decompress_block.c", "decompress/zstd_ddict.c",
            },
        });

        if (config.target.result.cpu.arch == .x86_64) {
            mod.addAssemblyFile(upstream.path("lib/decompress/huf_decompress_amd64.S"));
        } else {
            mod.addCMacro("ZSTD_DISABLE_ASM", "1");
        }
        mod.addIncludePath(lib_path);

        const lib = b.addLibrary(.{
            .name = "zstd",
            .root_module = mod,
        });
        lib.installHeader(lib_path.path(b, "zstd.h"), "zstd/zstd.h");
        lib.installHeader(lib_path.path(b, "zdict.h"), "zstd/zdict.h");
        lib.installHeader(lib_path.path(b, "zstd_errors.h"), "zstd/zstd_errors.h");

        return .{
            .dependency = upstream,
            .artifact = lib,
        };
    }
};

fn addFlagOptions(b: *std.Build) struct {
    compile_only: bool,
    skip_tests: bool,
    skip_cppcheck: bool,
    clean_cache: bool,
} {
    const compile_only = b.option(
        bool,
        "compile-only",
        "Skip copying legal documents to and compressing packaged directories",
    ) orelse false;

    const skip_tests = b.option(
        bool,
        "skip-tests",
        "Skip compilation of tests and disable test running",
    ) orelse false;

    const skip_cppcheck = b.option(
        bool,
        "skip-cppcheck",
        "Skip compilation of cppcheck and disable static analysis tooling",
    ) orelse false;

    const clean_cache = builtin.os.tag != .windows and b.option(
        bool,
        "clean-cache",
        "Clean the build cache with the clean step",
    ) orelse false;

    return .{
        .compile_only = compile_only,
        .skip_tests = skip_tests,
        .skip_cppcheck = skip_cppcheck,
        .clean_cache = clean_cache,
    };
}

const ExecutableBehavior = union(enum) {
    runnable: struct {
        cmd_name: []const u8,
        cmd_desc: []const u8,
    },
    standalone: void,
};

const TestArtifacts = struct {
    libcatch2: *std.Build.Step.Compile = undefined,
    runner_tests: *std.Build.Step.Compile = undefined,
    core_tests: *std.Build.Step.Compile = undefined,
    compiler_tests: *std.Build.Step.Compile = undefined,
    cli_tests: *std.Build.Step.Compile = undefined,

    pub fn configure(
        self: *const TestArtifacts,
        b: *std.Build,
        install: bool,
        cdb_steps: ?*std.ArrayList(*std.Build.Step),
    ) !void {
        if (install) {
            b.installArtifact(self.libcatch2);
            b.installArtifact(self.runner_tests);
            b.installArtifact(self.core_tests);
            b.installArtifact(self.compiler_tests);
            b.installArtifact(self.cli_tests);
        }

        if (cdb_steps) |cdb| {
            try cdb.append(b.allocator, &self.libcatch2.step);
            try cdb.append(b.allocator, &self.core_tests.step);
            try cdb.append(b.allocator, &self.compiler_tests.step);
            try cdb.append(b.allocator, &self.cli_tests.step);
        }

        const runners = [_]*std.Build.Step.Run{
            b.addRunArtifact(self.runner_tests),
            b.addRunArtifact(self.core_tests),
            b.addRunArtifact(self.compiler_tests),
            b.addRunArtifact(self.cli_tests),
        };

        const test_step = b.step("test", "Run all unit tests");
        for (runners) |runner| {
            runner.step.dependOn(b.getInstallStep());
            test_step.dependOn(&runner.step);
        }
    }
};

fn addArtifacts(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cxx_flags: []const []const u8,
    cdb_steps: ?*std.ArrayList(*std.Build.Step),
    skip_tests: bool,
    skip_cppcheck: bool,
    behavior: ?ExecutableBehavior = null,
    auto_install: bool = true,
}) !struct {
    libcore: *std.Build.Step.Compile,
    libcompiler: *std.Build.Step.Compile,
    libcli: *std.Build.Step.Compile,
    cli: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
} {
    const magic_enum = b.dependency("magic_enum", .{});
    const magic_enum_inc = magic_enum.path("include");

    // Shared core functionality
    const libcore = createLibrary(b, .{
        .name = "core",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{b.path(ProjectPaths.core.inc)},
        .system_include_paths = &.{magic_enum_inc},
        .cxx_files = try collectFiles(b, ProjectPaths.core.src, .{}),
        .cxx_flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libcore);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcore.step);

    // LLVM is compiled from source because I like burning compute or something
    const llvm: LibLLVM = try .build(b, .{
        .target = config.target,
        .auto_install = config.auto_install,
    });

    // The actual compiler static library
    const libcompiler = createLibrary(b, .{
        .name = "compiler",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
        },
        .system_include_paths = &.{magic_enum_inc},
        .link_libraries = &.{
            libcore,
            llvm.target_deps.libxml2.artifact,
            llvm.target_deps.zlib.artifact,
            llvm.target_deps.zstd.artifact,
            llvm.host_support,
        },
        .cxx_files = try collectFiles(b, ProjectPaths.compiler.src, .{}),
        .cxx_flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libcompiler);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcompiler.step);

    // The CLI library is stripped of main
    const libcli = createLibrary(b, .{
        .name = "cli",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.cli.inc),
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
        },
        .system_include_paths = &.{magic_enum_inc},
        .link_libraries = &.{libcompiler},
        .cxx_files = try collectFiles(b, ProjectPaths.cli.src, .{
            .dropped_files = &.{"main.cpp"},
        }),
        .cxx_flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libcli);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcli.step);

    // The shippable executable links only against libcli which has a transitive dep of the compiler
    const cli = createExecutable(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{b.path(ProjectPaths.cli.inc)},
        .cxx_files = &.{ProjectPaths.cli.src ++ "main.cpp"},
        .cxx_flags = config.cxx_flags,
        .link_libraries = &.{libcli},
        .behavior = config.behavior orelse .{
            .runnable = .{
                .cmd_name = "run",
                .cmd_desc = "Run conch with provided command line arguments",
            },
        },
    });
    if (config.auto_install) b.installArtifact(cli);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &cli.step);

    var tests: ?TestArtifacts = null;
    if (!config.skip_tests) {
        tests = .{};
        const test_runner = b.path(ProjectPaths.test_runner ++ "main.zig");

        const catch2 = b.dependency("catch2", .{});
        const catch2_inc = catch2.path("extras");
        tests.?.libcatch2 = createLibrary(b, .{
            .name = "catch2",
            .target = config.target,
            .optimize = .ReleaseSafe,
            .include_paths = &.{catch2_inc},
            .source_root = catch2.path("."),
            .cxx_files = &.{"extras/catch_amalgamated.cpp"},
            .cxx_flags = config.cxx_flags,
        });

        // The runner has standalone tests
        tests.?.runner_tests = b.addTest(.{
            .name = "runner_tests",
            .root_module = b.createModule(.{
                .root_source_file = test_runner,
                .optimize = config.optimize,
                .target = config.target,
                .link_libc = true,
            }),
        });

        const runner_cmd = b.addRunArtifact(tests.?.runner_tests);
        const runner_step = b.step("test-runner", "Run test instrumentation tests");
        runner_step.dependOn(&runner_cmd.step);

        // Core tests depend on the test runner but not LLVM
        tests.?.core_tests = createExecutable(b, .{
            .name = "core_tests",
            .zig_main = test_runner,
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.core.tests),
            },
            .system_include_paths = &.{
                catch2_inc,
                magic_enum_inc,
            },
            .cxx_files = try collectFiles(b, ProjectPaths.core.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ tests.?.libcatch2, libcore },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-core",
                    .cmd_desc = "Run core unit tests",
                },
            },
        });

        // Compiler tests can pull in core helpers
        tests.?.compiler_tests = createExecutable(b, .{
            .name = "compiler_tests",
            .zig_main = test_runner,
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.compiler.tests),
            },
            .system_include_paths = &.{
                catch2_inc,
                magic_enum_inc,
            },
            .cxx_files = try collectFiles(b, ProjectPaths.compiler.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ libcompiler, tests.?.libcatch2 },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-compiler",
                    .cmd_desc = "Run compiler unit tests",
                },
            },
        });

        // CLI tests can pull in core helpers
        tests.?.cli_tests = createExecutable(b, .{
            .name = "cli_tests",
            .zig_main = b.path(ProjectPaths.test_runner ++ "main.zig"),
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.cli.inc),
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.cli.tests),
            },
            .system_include_paths = &.{
                catch2_inc,
                magic_enum_inc,
            },
            .cxx_files = try collectFiles(b, ProjectPaths.cli.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{
                libcompiler,
                libcli,
                tests.?.libcatch2,
            },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-cli",
                    .cmd_desc = "Run CLI unit tests",
                },
            },
        });

        try tests.?.configure(b, config.auto_install, config.cdb_steps);
    }

    const cppcheck = if (config.skip_cppcheck) null else try compileCppcheck(b, config.target);

    return .{
        .libcore = libcore,
        .libcompiler = libcompiler,
        .libcli = libcli,
        .cli = cli,
        .tests = tests,
        .cppcheck = cppcheck,
    };
}

/// Compiles cppcheck from source using the flags given by:
/// https://github.com/danmar/cppcheck#g-for-experts
fn compileCppcheck(b: *std.Build, target: std.Build.ResolvedTarget) !*std.Build.Step.Compile {
    const cppcheck = b.dependency("cppcheck", .{});
    const cppcheck_includes: []const std.Build.LazyPath = &.{
        cppcheck.path("externals"),
        cppcheck.path("externals/simplecpp"),
        cppcheck.path("externals/tinyxml2"),
        cppcheck.path("externals/picojson"),
        cppcheck.path("lib"),
        cppcheck.path("frontend"),
    };

    const cppcheck_sources = [_][]const u8{
        "externals/simplecpp/simplecpp.cpp", "externals/tinyxml2/tinyxml2.cpp",
        "frontend/frontend.cpp",             "cli/cmdlineparser.cpp",
        "cli/cppcheckexecutor.cpp",          "cli/executor.cpp",
        "cli/filelister.cpp",                "cli/main.cpp",
        "cli/processexecutor.cpp",           "cli/sehwrapper.cpp",
        "cli/signalhandler.cpp",             "cli/singleexecutor.cpp",
        "cli/stacktrace.cpp",                "cli/threadexecutor.cpp",
        "lib/addoninfo.cpp",                 "lib/analyzerinfo.cpp",
        "lib/astutils.cpp",                  "lib/check.cpp",
        "lib/check64bit.cpp",                "lib/checkassert.cpp",
        "lib/checkautovariables.cpp",        "lib/checkbool.cpp",
        "lib/checkbufferoverrun.cpp",        "lib/checkclass.cpp",
        "lib/checkcondition.cpp",            "lib/checkers.cpp",
        "lib/checkersidmapping.cpp",         "lib/checkersreport.cpp",
        "lib/checkexceptionsafety.cpp",      "lib/checkfunctions.cpp",
        "lib/checkinternal.cpp",             "lib/checkio.cpp",
        "lib/checkleakautovar.cpp",          "lib/checkmemoryleak.cpp",
        "lib/checknullpointer.cpp",          "lib/checkother.cpp",
        "lib/checkpostfixoperator.cpp",      "lib/checksizeof.cpp",
        "lib/checkstl.cpp",                  "lib/checkstring.cpp",
        "lib/checktype.cpp",                 "lib/checkuninitvar.cpp",
        "lib/checkunusedfunctions.cpp",      "lib/checkunusedvar.cpp",
        "lib/checkvaarg.cpp",                "lib/clangimport.cpp",
        "lib/color.cpp",                     "lib/cppcheck.cpp",
        "lib/ctu.cpp",                       "lib/errorlogger.cpp",
        "lib/errortypes.cpp",                "lib/findtoken.cpp",
        "lib/forwardanalyzer.cpp",           "lib/fwdanalysis.cpp",
        "lib/importproject.cpp",             "lib/infer.cpp",
        "lib/keywords.cpp",                  "lib/library.cpp",
        "lib/mathlib.cpp",                   "lib/path.cpp",
        "lib/pathanalysis.cpp",              "lib/pathmatch.cpp",
        "lib/platform.cpp",                  "lib/preprocessor.cpp",
        "lib/programmemory.cpp",             "lib/regex.cpp",
        "lib/reverseanalyzer.cpp",           "lib/sarifreport.cpp",
        "lib/settings.cpp",                  "lib/standards.cpp",
        "lib/summaries.cpp",                 "lib/suppressions.cpp",
        "lib/symboldatabase.cpp",            "lib/templatesimplifier.cpp",
        "lib/timer.cpp",                     "lib/token.cpp",
        "lib/tokenize.cpp",                  "lib/tokenlist.cpp",
        "lib/utils.cpp",                     "lib/valueflow.cpp",
        "lib/vf_analyzers.cpp",              "lib/vf_common.cpp",
        "lib/vf_settokenvalue.cpp",          "lib/vfvalue.cpp",
    };

    // The path needs to be fixed on windows due to cppcheck internals
    const cfg_path = blk: {
        const raw_cfg_path = try cppcheck.path(".").getPath3(b, null).toString(b.allocator);
        if (builtin.os.tag == .windows) {
            break :blk try std.mem.replaceOwned(u8, b.allocator, raw_cfg_path, "\\", "/");
        }
        break :blk raw_cfg_path;
    };

    const files_dir = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{cfg_path},
    );

    return createExecutable(b, .{
        .name = "cppcheck",
        .target = target,
        .optimize = .ReleaseSafe,
        .include_paths = cppcheck_includes,
        .source_root = cppcheck.path("."),
        .cxx_files = &cppcheck_sources,
        .cxx_flags = &.{ files_dir, "-Uunix", "-std=c++11" },
    });
}

const SystemLibraries = struct {
    search_paths: []const std.Build.LazyPath,
    libs: []const []const u8,
};

fn createLibrary(b: *std.Build, config: struct {
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_paths: []const std.Build.LazyPath,
    system_include_paths: ?[]const std.Build.LazyPath = null,
    source_root: ?std.Build.LazyPath = null,
    link_libraries: ?[]const *std.Build.Step.Compile = null,
    system_libraries: ?SystemLibraries = null,
    cxx_files: []const []const u8,
    cxx_flags: []const []const u8,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    for (config.include_paths) |inc_path| {
        mod.addIncludePath(inc_path);
    }

    if (config.system_include_paths) |system_includes| {
        for (system_includes) |inc_path| {
            mod.addSystemIncludePath(inc_path);
        }
    }

    if (config.link_libraries) |link_libraries| {
        for (link_libraries) |lib| {
            mod.linkLibrary(lib);
        }
    }

    mod.addCSourceFiles(.{
        .root = config.source_root,
        .files = config.cxx_files,
        .flags = config.cxx_flags,
        .language = .cpp,
    });

    if (config.system_libraries) |libs| {
        for (libs.search_paths) |path| {
            mod.addLibraryPath(path);
        }

        for (libs.libs) |lib| {
            mod.linkSystemLibrary(lib, .{
                .preferred_link_mode = .static,
            });
        }
    }

    return b.addLibrary(.{
        .name = config.name,
        .root_module = mod,
    });
}

fn createExecutable(b: *std.Build, config: struct {
    name: []const u8,
    zig_main: ?std.Build.LazyPath = null,
    target: ?std.Build.ResolvedTarget,
    optimize: ?std.builtin.OptimizeMode,
    include_paths: []const std.Build.LazyPath,
    system_include_paths: ?[]const std.Build.LazyPath = null,
    source_root: ?std.Build.LazyPath = null,
    cxx_files: []const []const u8,
    cxx_flags: []const []const u8,
    link_libraries: []const *std.Build.Step.Compile = &.{},
    system_libraries: ?SystemLibraries = null,
    behavior: ExecutableBehavior = .standalone,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .root_source_file = config.zig_main,
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    for (config.include_paths) |include| {
        mod.addIncludePath(include);
    }

    if (config.system_include_paths) |system_includes| {
        for (system_includes) |inc_path| {
            mod.addSystemIncludePath(inc_path);
        }
    }

    for (config.link_libraries) |library| {
        mod.linkLibrary(library);
    }

    mod.addCSourceFiles(.{
        .root = config.source_root,
        .files = config.cxx_files,
        .flags = config.cxx_flags,
        .language = .cpp,
    });

    if (config.system_libraries) |libs| {
        for (libs.search_paths) |path| {
            mod.addLibraryPath(path);
        }

        for (libs.libs) |lib| {
            mod.linkSystemLibrary(lib, .{
                .preferred_link_mode = .static,
            });
        }
    }

    const exe = b.addExecutable(.{
        .name = config.name,
        .root_module = mod,
    });

    switch (config.behavior) {
        .runnable => |run| {
            const run_cmd = b.addRunArtifact(exe);
            run_cmd.step.dependOn(b.getInstallStep());

            if (b.args) |args| {
                run_cmd.addArgs(args);
            }

            const run_step = b.step(run.cmd_name, run.cmd_desc);
            run_step.dependOn(&run_cmd.step);
        },
        .standalone => {},
    }

    return exe;
}

const CdbGenerator = struct {
    const cdb_filename = "compile_commands.json";
    const cdb_frags_dirname = "cdb-frags";

    const CdbFileInfo = struct {
        file: []const u8,
    };
    const ParsedCdbFileInfo = std.json.Parsed(CdbFileInfo);

    const FragInfo = struct {
        name: []const u8,
        mtime: i128,
    };

    step: std.Build.Step,
    output_file: std.Build.GeneratedFile,

    pub fn init(b: *std.Build) *CdbGenerator {
        const self = b.allocator.create(CdbGenerator) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "generate-cdb",
                .owner = b,
                .makeFn = generateCdb,
            }),
            .output_file = .{ .step = &self.step },
        };
        return self;
    }

    pub fn getCdbPath(self: *const CdbGenerator) std.Build.LazyPath {
        return .{ .generated = .{ .file = &self.output_file } };
    }

    fn generateCdb(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *CdbGenerator = @fieldParentPtr("step", step);

        const b = step.owner;
        const allocator = b.allocator;
        const cache_root = b.cache_root.handle;

        self.output_file.path = getCacheRelativePath(b, &.{cdb_filename});
        try cache_root.makePath(cdb_frags_dirname);
        var newest_frags: std.StringHashMap(FragInfo) = .init(allocator);

        var dir = try cache_root.openDir(cdb_frags_dirname, .{ .iterate = true });
        defer dir.close();
        var dir_iter = dir.iterate();

        // The frags balloon like crazy so cleaning up proactively is needed
        var old_frags: std.ArrayList([]const u8) = .empty;

        // Hashed updates are generated by the compiler, so grab the most recent for the cdb
        const file_buf = try allocator.alloc(u8, 64 * 1024);
        while (try dir_iter.next()) |entry| {
            if (entry.kind != .file) continue;
            const entry_name = b.dupe(entry.name);
            const stat = try dir.statFile(entry_name);
            const first_dot = std.mem.indexOf(u8, entry_name, ".") orelse {
                try old_frags.append(allocator, entry_name);
                continue;
            };
            const base_name = entry_name[0 .. first_dot + 4];

            const entry_contents = try dir.readFile(entry_name, file_buf);
            const trimmed = std.mem.trimEnd(u8, entry_contents, ",\n\r\t");
            const parsed: ParsedCdbFileInfo = std.json.parseFromSlice(
                CdbFileInfo,
                allocator,
                trimmed,
                .{ .ignore_unknown_fields = true },
            ) catch continue;
            const ref_path = parsed.value.file;
            const absolute_ref_path = if (std.fs.path.isAbsolute(ref_path))
                ref_path
            else
                try b.build_root.join(allocator, &.{ref_path});

            // Orphaned files should be removed too
            std.fs.accessAbsolute(absolute_ref_path, .{}) catch {
                try old_frags.append(allocator, entry_name);
                continue;
            };

            const gop = try newest_frags.getOrPut(base_name);
            if (!gop.found_existing) {
                gop.value_ptr.* = .{
                    .name = entry_name,
                    .mtime = stat.mtime,
                };
            } else {
                if (stat.mtime > gop.value_ptr.mtime) {
                    try old_frags.append(allocator, gop.value_ptr.name);
                    gop.value_ptr.name = entry_name;
                    gop.value_ptr.mtime = stat.mtime;
                } else {
                    try old_frags.append(allocator, entry_name);
                }
            }
        }

        for (old_frags.items) |old| {
            dir.deleteFile(old) catch continue;
        }

        var frag_iter = newest_frags.valueIterator();
        var first = true;
        const cdb = try cache_root.createFile(cdb_filename, .{});
        defer cdb.close();

        _ = try cdb.write("[");
        while (frag_iter.next()) |info| {
            if (!first) _ = try cdb.write(",\n");
            first = false;

            const fpath = b.pathJoin(&.{ cdb_frags_dirname, info.name });
            const contents = try cache_root.readFile(fpath, file_buf);
            const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
            _ = try cdb.write(trimmed);
        }
        _ = try cdb.write("]");
    }
};

fn addTooling(b: *std.Build, config: struct {
    cdb_gen: *CdbGenerator,
    cli: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
    clean_cache: bool,
}) !void {
    const tooling_sources = try collectToolingFiles(b);

    const cdb_step = b.step("cdb", "Generate " ++ CdbGenerator.cdb_filename);
    cdb_step.dependOn(&config.cdb_gen.step);
    b.getInstallStep().dependOn(&config.cdb_gen.step);

    if (findProgram(b, "clang-format")) |clang_format| {
        try addFmtStep(b, .{
            .tooling_sources = tooling_sources,
            .clang_format = clang_format,
        });
    }

    if (config.cppcheck) |cppcheck| {
        const check_step = try addStaticAnalysisStep(b, .{
            .tooling_sources = tooling_sources,
            .cppcheck = cppcheck,
            .cdb_gen = config.cdb_gen,
        });
        check_step.dependOn(&config.cdb_gen.step);
    }

    const cloc: *LOCCounter = .init(b);
    const cloc_step = b.step("cloc", "Count lines of code across the project");
    cloc_step.dependOn(&cloc.step);

    if (config.tests) |tests| {
        try addLLDBStep(b, .{
            .cli = config.cli,
            .tests = tests,
        });
    }

    const clean_step = b.step("clean", "Remove all emitted artifacts");
    const remove_install = b.addRemoveDirTree(b.path(getPrefixRelativePath(b, &.{})));
    clean_step.dependOn(&remove_install.step);
    if (config.clean_cache) {
        const remove_cache = b.addRemoveDirTree(b.path(getCacheRelativePath(b, &.{})));
        clean_step.dependOn(&remove_cache.step);
    }
}

fn addFmtStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    clang_format: []const u8,
}) !void {
    const zig_paths: []const []const u8 = &.{
        "build.zig",
        "build.zig.zon",
        ProjectPaths.test_runner ++ "main.zig",
    };
    const build_fmt = b.addFmt(.{ .paths = zig_paths });
    const build_fmt_check = b.addFmt(.{ .paths = zig_paths, .check = true });

    const fmt = b.addSystemCommand(&.{config.clang_format});
    fmt.addArg("-i");
    fmt.addArgs(config.tooling_sources);
    const fmt_step = b.step("fmt", "Format all project files");
    fmt_step.dependOn(&fmt.step);
    fmt_step.dependOn(&build_fmt.step);

    const fmt_check = b.addSystemCommand(&.{config.clang_format});
    fmt_check.addArgs(&.{ "--dry-run", "--Werror" });
    fmt_check.addArgs(config.tooling_sources);
    const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
    fmt_check_step.dependOn(&fmt_check.step);
    fmt_check_step.dependOn(&build_fmt_check.step);
}

fn addStaticAnalysisStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    cppcheck: *std.Build.Step.Compile,
    cdb_gen: *CdbGenerator,
}) !*std.Build.Step {
    const check_step = b.step("check", "Run static analysis on all project files");
    const cppcheck = b.addRunArtifact(config.cppcheck);

    const installed_cppcheck_cache_path = getCacheRelativePath(b, &.{"cppcheck"});
    cppcheck.addArg("--inline-suppr");
    cppcheck.addPrefixedFileArg("--project=", config.cdb_gen.getCdbPath());
    const cppcheck_cache = cppcheck.addPrefixedOutputDirectoryArg(
        "--cppcheck-build-dir=",
        installed_cppcheck_cache_path,
    );
    cppcheck.addArg("--check-level=exhaustive");
    cppcheck.addArgs(&.{ "--error-exitcode=1", "--enable=all" });
    cppcheck.addArgs(&.{
        "-icatch_amalgamated.cpp",
        "--suppress=*:catch_amalgamated.hpp",
        "--suppress=*:magic_enum.hpp",
    });

    const suppressions: []const []const u8 = &.{
        "checkersReport",
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
    };

    inline for (suppressions) |suppression| {
        cppcheck.addArg("--suppress=" ++ suppression);
    }

    cppcheck.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.core.tests));
    cppcheck.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.compiler.tests));
    cppcheck.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.cli.tests));

    const cppcheck_cache_install = b.addInstallDirectory(.{
        .source_dir = cppcheck_cache,
        .install_dir = .{ .custom = ".." },
        .install_subdir = installed_cppcheck_cache_path,
    });

    cppcheck_cache_install.step.dependOn(&config.cppcheck.step);
    check_step.dependOn(&cppcheck_cache_install.step);
    check_step.dependOn(&cppcheck.step);
    return check_step;
}

fn addLLDBStep(b: *std.Build, config: struct {
    cli: *std.Build.Step.Compile,
    tests: TestArtifacts,
}) !void {
    if (builtin.os.tag == .windows) return;
    const error_msg =
        \\LLDB is is LLVM's CLI debugger whose source code can be found here:
        \\  https://github.com/llvm/llvm-project
        \\
        \\It is available on most major platforms through their corresponding package managers.
    ;

    const dbg = b.step("dbg", "Debug the main executable with lldb");
    const dbg_cli = b.step("dbg-cli", "Debug the cli tests with lldb");
    const dbg_compiler = b.step("dbg-compiler", "Debug the compiler tests with lldb");
    const dbg_core = b.step("dbg-core", "Debug the core tests with lldb");
    const dbg_runner = b.step("dbg-runner", "Debug the runner tests with lldb");
    const steps = [_]struct { *std.Build.Step, *std.Build.Step.Compile }{
        .{ dbg, config.cli },
        .{ dbg_cli, config.tests.cli_tests },
        .{ dbg_compiler, config.tests.compiler_tests },
        .{ dbg_core, config.tests.core_tests },
        .{ dbg_runner, config.tests.runner_tests },
    };

    for (steps) |step| {
        if (findProgram(b, "lldb")) |lldb| {
            var debugger: *std.Build.Step.Run = undefined;

            if (findProgram(b, "coreutils")) |coreutils| {
                debugger = b.addSystemCommand(&.{coreutils});
                debugger.addArgs(&.{ "--coreutils-prog=env", "-i", lldb });
            } else if (findProgram(b, "env")) |env| {
                debugger = b.addSystemCommand(&.{env});
                debugger.addArgs(&.{ "-i", lldb });
            } else {
                debugger = b.addSystemCommand(&.{lldb});
                debugger = b.addSystemCommand(&.{lldb});
            }

            debugger.addArtifactArg(step.@"1");
            step.@"0".dependOn(&debugger.step);
        } else {
            try step.@"0".addError(error_msg, .{});
        }
    }
}

const LOCCounter = struct {
    const LOCResult = struct {
        counts: std.StringHashMap(struct {
            line_count: usize,
            frequency: usize,
        }),
        total_line_count: usize,
        file_count: usize,

        pub fn init(allocator: std.mem.Allocator) LOCResult {
            return .{
                .counts = .init(allocator),
                .total_line_count = 0,
                .file_count = 0,
            };
        }

        // Adds a file to the counts, grouping by un-dotted extension
        pub fn logFile(self: *LOCResult, file_path: []const u8, line_count: usize) !void {
            const ext = std.fs.path.extension(file_path)[1..];
            const gop = try self.counts.getOrPut(ext);

            if (gop.found_existing) {
                gop.value_ptr.line_count += line_count;
                gop.value_ptr.frequency += 1;
            } else {
                gop.value_ptr.* = .{
                    .line_count = line_count,
                    .frequency = 1,
                };
            }
            self.file_count += 1;
            self.total_line_count += line_count;
        }

        pub fn print(self: *const LOCResult) !void {
            const stdout_handle = std.fs.File.stdout();
            var stdout_buf: [1024]u8 = undefined;
            var stdout_writer = stdout_handle.writer(&stdout_buf);
            const stdout = &stdout_writer.interface;

            try stdout.print("Scanned {d} total files:\n", .{self.file_count});

            var count_iter = self.counts.iterator();
            while (count_iter.next()) |entry| {
                try stdout.print("  {d} total {s} files: {d} LOC\n", .{
                    entry.value_ptr.frequency,
                    entry.key_ptr.*,
                    entry.value_ptr.line_count,
                });
            }
            try stdout.print("Total: {d} LOC\n", .{self.total_line_count});

            try stdout.flush();
        }
    };

    step: std.Build.Step,

    pub fn init(
        b: *std.Build,
    ) *LOCCounter {
        const self = b.allocator.create(LOCCounter) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "cloc",
                .owner = b,
                .makeFn = count,
            }),
        };
        return self;
    }

    fn count(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const b = step.owner;

        const extensions = [_][]const u8{ ".cpp", ".hpp", ".zig", ".conch" };
        var files: std.ArrayList([]const u8) = .empty;

        try files.appendSlice(
            b.allocator,
            try collectFiles(b, "packages", .{
                .allowed_extensions = &extensions,
                .extra_files = &.{"build.zig"},
            }),
        );

        try files.appendSlice(
            b.allocator,
            try collectFiles(b, "apps", .{ .allowed_extensions = &extensions }),
        );

        const build_dir = b.build_root.handle;
        const buffer = try b.allocator.alloc(u8, 100 * 1024);
        var result: LOCResult = .init(b.allocator);

        for (files.items) |file| {
            const contents = try build_dir.readFile(file, buffer);
            var it = std.mem.tokenizeAny(u8, contents, "\r\n");

            var lines: usize = 0;
            while (it.next()) |line| {
                const trimmed = std.mem.trim(u8, line, " \t");
                if (trimmed.len > 0 and !std.mem.startsWith(u8, trimmed, "//")) {
                    lines += 1;
                }
            }

            try result.logFile(file, lines);
        }

        try result.print();
    }
};

const target_queries = [_]std.Target.Query{
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .macos },

    .{ .cpu_arch = .x86, .os_tag = .linux },
    .{ .cpu_arch = .x86_64, .os_tag = .linux },
    .{ .cpu_arch = .aarch64, .os_tag = .linux },
    .{ .cpu_arch = .powerpc, .os_tag = .linux },
    .{ .cpu_arch = .powerpc64, .os_tag = .linux },
    .{ .cpu_arch = .powerpc64le, .os_tag = .linux },
    .{ .cpu_arch = .riscv32, .os_tag = .linux },
    .{ .cpu_arch = .riscv64, .os_tag = .linux },
    .{ .cpu_arch = .loongarch64, .os_tag = .linux },

    .{ .cpu_arch = .x86_64, .os_tag = .freebsd },
    .{ .cpu_arch = .aarch64, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc64, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc64le, .os_tag = .freebsd },
    .{ .cpu_arch = .riscv64, .os_tag = .freebsd },

    .{ .cpu_arch = .x86, .os_tag = .netbsd },
    .{ .cpu_arch = .x86_64, .os_tag = .netbsd },
    .{ .cpu_arch = .aarch64, .os_tag = .netbsd },

    .{ .cpu_arch = .x86, .os_tag = .windows },
    .{ .cpu_arch = .x86_64, .os_tag = .windows },
    .{ .cpu_arch = .aarch64, .os_tag = .windows },
};

fn addPackageStep(b: *std.Build, config: struct {
    compile_only: bool,
    cxx_flags: []const []const u8,
}) !void {
    // TODO, support packaging correctly with LLVM
    const package_step = b.step("package", "Build the artifacts for packaging");
    if (true) {
        return package_step.addError("Packing is currently unsupported as LLVM integration is a WIP", .{});
    }

    const uncompressed_package_dir: []const []const u8 = &.{ "package", "uncompressed" };
    const compressed_package_dir: []const []const u8 = &.{ "package", "compressed" };

    var zon_buf: [2 * 1024]u8 = undefined;
    const raw_zon_contents = try b.build_root.handle.readFile("build.zig.zon", &zon_buf);
    zon_buf[raw_zon_contents.len] = 0;
    const zon_contents = zon_buf[0..raw_zon_contents.len :0];

    const parsed = try std.zon.parse.fromSlice(
        struct { version: []const u8 },
        b.allocator,
        zon_contents,
        null,
        .{ .ignore_unknown_fields = true },
    );
    const version = parsed.version;

    for (target_queries) |query| {
        const resolved_target = b.resolveTargetQuery(query);
        const artifacts = try addArtifacts(b, .{
            .target = resolved_target,
            .optimize = .ReleaseFast,
            .cxx_flags = config.cxx_flags,
            .cdb_steps = null,
            .skip_tests = true,
            .skip_cppcheck = true,
            .behavior = .standalone,
            .auto_install = false,
        });

        std.debug.assert(artifacts.tests == null);

        try configurePackArtifacts(b, .{
            .artifacts = &.{ artifacts.libcompiler, artifacts.libcli, artifacts.cli },
            .target = resolved_target,
            .version = version,
        });

        const package_artifact_dirname = try std.fmt.allocPrint(
            b.allocator,
            "conch-{s}-{s}",
            .{ try query.zigTriple(b.allocator), version },
        );

        const package_artifact_dir_path = b.pathJoin(uncompressed_package_dir ++ .{package_artifact_dirname});
        const platform = b.addInstallArtifact(
            artifacts.cli,
            .{
                .dest_dir = .{
                    .override = .{ .custom = package_artifact_dir_path },
                },
            },
        );
        package_step.dependOn(&platform.step);

        if (!config.compile_only) {
            const tar = findProgram(b, "tar");
            const zip = findProgram(b, "zip");
            if (zip == null or tar == null) {
                return package_step.addError(
                    \\Packaging cannot run without zip and tar commands
                    \\  zip: {s}
                    \\  tar: {s}
                , .{ zip orelse "null", tar orelse "null" });
            }

            const legal_paths = [_]struct { std.Build.LazyPath, []const u8 }{
                .{ b.path("LICENSE"), "LICENSE" },
                .{ b.path("README.md"), "README.md" },
                .{ b.path(".github/CHANGELOG.md"), "CHANGELOG.md" },
            };

            var file_installs: [legal_paths.len]*std.Build.Step = undefined;
            for (legal_paths, 0..) |path, i| {
                const install_file_step = b.addInstallFileWithDir(
                    path.@"0",
                    .{ .custom = package_artifact_dir_path },
                    path.@"1",
                );
                package_step.dependOn(&install_file_step.step);
                file_installs[i] = &install_file_step.step;
            }

            // Zip is only needed on windows
            if (query.os_tag.? == .windows) {
                const zip_filename = try std.fmt.allocPrint(
                    b.allocator,
                    "{s}.zip",
                    .{package_artifact_dirname},
                );

                const zipper = b.addSystemCommand(&.{ zip.?, "-r" });
                const output_zip = zipper.addOutputFileArg(zip_filename);
                zipper.addArg(package_artifact_dirname);
                zipper.setCwd(b.path(getPrefixRelativePath(b, uncompressed_package_dir)));
                _ = zipper.captureStdErr();

                zipper.step.dependOn(&platform.step);
                package_step.dependOn(&zipper.step);
                for (file_installs) |step| {
                    zipper.step.dependOn(step);
                }

                const copy_zip = b.addInstallFileWithDir(
                    output_zip,
                    .{ .custom = b.pathJoin(compressed_package_dir) },
                    zip_filename,
                );
                package_step.dependOn(&copy_zip.step);
            }

            // All platforms get an archive because I'm nice
            const tar_filename = try std.fmt.allocPrint(
                b.allocator,
                "{s}.tar.gz",
                .{package_artifact_dirname},
            );

            const archiver = b.addSystemCommand(&.{ tar.?, "-czf" });
            const output_tar = archiver.addOutputFileArg(tar_filename);
            archiver.addArg("-C");
            archiver.addArg(getPrefixRelativePath(b, uncompressed_package_dir));
            archiver.addArg(package_artifact_dirname);
            _ = archiver.captureStdErr();

            archiver.step.dependOn(&platform.step);
            package_step.dependOn(&archiver.step);
            for (file_installs) |step| {
                archiver.step.dependOn(step);
            }

            const copy_tar = b.addInstallFileWithDir(
                output_tar,
                .{ .custom = b.pathJoin(compressed_package_dir) },
                tar_filename,
            );
            package_step.dependOn(&copy_tar.step);
        }
    }
}

fn configurePackArtifacts(b: *std.Build, config: struct {
    artifacts: []const *std.Build.Step.Compile,
    target: std.Build.ResolvedTarget,
    version: []const u8,
}) !void {
    for (config.artifacts) |artifact| {
        artifact.root_module.strip = true;
        artifact.out_filename = blk: {
            if (config.target.result.os.tag == .windows) {
                break :blk try std.fmt.allocPrint(
                    b.allocator,
                    "{s}-{s}.exe",
                    .{ artifact.name, config.version },
                );
            } else {
                break :blk try std.fmt.allocPrint(
                    b.allocator,
                    "{s}-{s}",
                    .{ artifact.name, config.version },
                );
            }
        };
    }
}

fn collectFiles(
    b: *std.Build,
    directory: []const u8,
    options: struct {
        allowed_extensions: []const []const u8 = &.{".cpp"},
        dropped_files: ?[]const [:0]const u8 = null,
        extra_files: ?[]const []const u8 = null,
    },
) ![]const []const u8 {
    var dir = try b.build_root.handle.openDir(directory, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    var paths: std.ArrayList([]const u8) = .empty;
    collector: while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        for (options.allowed_extensions) |ext| {
            if (std.mem.endsWith(u8, entry.basename, ext)) break;
        } else continue :collector;

        if (options.dropped_files) |drop| for (drop) |drop_file| {
            if (std.mem.eql(u8, drop_file, entry.basename)) continue :collector;
        };

        const full_path = b.pathJoin(&.{ directory, entry.path });
        try paths.append(b.allocator, full_path);
    }

    if (options.extra_files) |extra_files| {
        try paths.appendSlice(b.allocator, extra_files);
    }
    return paths.items;
}

fn collectToolingFiles(b: *std.Build) ![]const []const u8 {
    return std.mem.concat(b.allocator, []const u8, &.{
        try ProjectPaths.compiler.files(b),
        try ProjectPaths.cli.files(b),
        try ProjectPaths.core.files(b),
        try collectFiles(b, ProjectPaths.test_runner, .{ .allowed_extensions = &.{".cpp"} }),
    });
}

/// Resolves the relative path with its root at the cache directory
fn getCacheRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    return b.cache_root.join(b.allocator, paths) catch @panic("OOM");
}

/// Resolves the relative path with its root at the installation directory
fn getPrefixRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    return b.pathJoin(std.mem.concat(
        b.allocator,
        []const u8,
        &.{ &.{std.fs.path.basename(b.install_prefix)}, paths },
    ) catch @panic("OOM"));
}

fn findProgram(b: *std.Build, cmd: []const u8) ?[]const u8 {
    return b.findProgram(&.{cmd}, &.{}) catch null;
}
