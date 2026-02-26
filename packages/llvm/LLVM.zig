//! LLVM Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

const support = @import("sources/support.zig");
const tblgen = @import("sources/tblgen.zig");
const ir = @import("sources/ir.zig");
const machine_code = @import("sources/machine_code.zig");
const object = @import("sources/object.zig");

const ThirdParty = struct {
    const siphash_inc = "third-party/siphash/include";
};

const optimize: std.builtin.OptimizeMode = .ReleaseSafe;

const Dependencies = struct {
    const Dependency = struct {
        dependency: *std.Build.Dependency,
        artifact: *std.Build.Step.Compile,
    };

    zlib: Dependency,
    libxml2: Dependency,
    zstd: Dependency,
};

const Platform = enum { host, target };

/// A minimal set of artifacts, compiled for the host arch.
///
/// These are used for the generation of core files for the LLVM pipeline.
/// A distinction between minimal and full must be made to prevent a circular dependency.
const MinimalArtifacts = struct {
    tblgen: *std.Build.Step.Compile = undefined,
    deps: Dependencies = undefined,
    demangle: *std.Build.Step.Compile = undefined,
    support: *std.Build.Step.Compile = undefined,

    /// GenVT generated include.
    gen_vt: *std.Build.Step.WriteFile,
};

/// Artifacts compiled for the actual target
const TargetArtifacts = struct {
    const MachineCode = struct {
        mc: *std.Build.Step.Compile = undefined,
        parser: *std.Build.Step.Compile = undefined,
        disassembler: *std.Build.Step.Compile = undefined,
    };

    const TextAPI = struct {
        text_api: *std.Build.Step.Compile = undefined,
        binary_reader: *std.Build.Step.Compile = undefined,
    };

    deps: Dependencies = undefined,
    demangle: *std.Build.Step.Compile = undefined,
    support: *std.Build.Step.Compile = undefined,
    target_parser: *std.Build.Step.Compile = undefined,
    bitstream_reader: *std.Build.Step.Compile = undefined,
    binary_format: *std.Build.Step.Compile = undefined,
    remarks: *std.Build.Step.Compile = undefined,
    core: *std.Build.Step.Compile = undefined,

    bitcode: struct {
        reader: *std.Build.Step.Compile = undefined,
        writer: *std.Build.Step.Compile = undefined,
    } = .{},

    machine_code: MachineCode = undefined,
    asm_parser: *std.Build.Step.Compile = undefined,
    ir_reader: *std.Build.Step.Compile = undefined,
    text_api: TextAPI = undefined,
    object: *std.Build.Step.Compile = undefined,

    /// Registry of generated intrinsics
    intrinsics_gen: *std.Build.Step.WriteFile = undefined,

    /// Registry of target-specific generated files
    target_parser_gen: *std.Build.Step.WriteFile = undefined,

    fn artifactArray(self: *const TargetArtifacts) [18]*std.Build.Step.Compile {
        return .{
            self.deps.zlib.artifact,
            self.deps.libxml2.artifact,
            self.deps.zstd.artifact,
            self.demangle,
            self.support,
            self.target_parser,
            self.bitstream_reader,
            self.binary_format,
            self.remarks,
            self.core,
            self.bitcode.reader,
            self.machine_code.mc,
            self.machine_code.disassembler,
            self.machine_code.parser,
            self.asm_parser,
            self.ir_reader,
            self.text_api.text_api,
            self.object,
        };
    }
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

const common_llvm_cxx_flags = [_][]const u8{
    "-std=c++17",
    "-fno-exceptions",
    "-fno-rtti",
};

const enabled_targets = &[_][]const u8{
    "X86",
    "AArch64",
    "ARM",
    "RISCV",
    "WebAssembly",
    "Xtensa",
    "PowerPC",
    "LoongArch",
};

/// Converts the enabled target into its Abbr, usually a no-op.
fn targetAbbr(enabled_target: []const u8) []const u8 {
    if (std.mem.eql(u8, enabled_target, "PowerPC")) {
        return "PPC";
    }
    return enabled_target;
}

const Self = @This();

b: *std.Build,
llvm: struct {
    upstream: *std.Build.Dependency,
    root: std.Build.LazyPath,
    llvm_include: std.Build.LazyPath,
    vcs_revision: *std.Build.Step.ConfigHeader,
},

minimal_artifacts: MinimalArtifacts,

/// Used to convert the actual target 'td's into 'inc's
full_tblgen: *std.Build.Step.Compile = undefined,

target_artifacts: TargetArtifacts = .{},

/// The target system to compile to
target: std.Build.ResolvedTarget,

pub fn init(b: *std.Build, target: std.Build.ResolvedTarget) Self {
    const upstream = b.dependency("llvm", .{});
    return .{
        .b = b,
        .llvm = .{
            .upstream = upstream,
            .root = upstream.path("."),
            .llvm_include = upstream.path("llvm/include"),
            .vcs_revision = b.addConfigHeader(.{
                .style = .blank,
                .include_path = "llvm/Support/VCSRevision.h",
            }, .{
                .LLVM_REVISION = "git-" ++ version_str ++ "-2078da43e25a4623cab2d0d60decddf709aaea28",
            }),
        },
        .minimal_artifacts = .{
            .gen_vt = b.addWriteFiles(),
        },
        .target = target,
    };
}

pub fn build(self: *Self, link_test_config: ?struct {
    auto_install: bool,
    cxx_flags: []const []const u8,
}) !void {
    const b = self.b;

    // Host Dependencies for TableGen
    self.minimal_artifacts.deps = try self.compileDeps(b.graph.host);
    self.minimal_artifacts.demangle = self.compileDemangle(b.graph.host);
    self.minimal_artifacts.support = try self.compileSupport(.{
        .platform = .host,
        .deps = self.minimal_artifacts.deps,
        .minimal = true,
    });

    self.minimal_artifacts.tblgen = self.compileTblgen(.{
        .support_lib = self.minimal_artifacts.support,
        .minimal = true,
    });

    _ = self.synthesizeHeader(self.minimal_artifacts.gen_vt, .{
        .tblgen = self.minimal_artifacts.tblgen,
        .name = "GenVT",
        .td_file = "llvm/include/llvm/CodeGen/ValueTypes.td",
        .instruction = .{ .action = "-gen-vt" },
        .virtual_path = "llvm/CodeGen/GenVT.inc",
    });

    self.full_tblgen = self.compileTblgen(.{
        .support_lib = self.minimal_artifacts.support,
        .minimal = false,
    });
    self.full_tblgen.root_module.addIncludePath(self.minimal_artifacts.gen_vt.getDirectory());

    // Target dependencies must be built fully
    self.target_artifacts.deps = try self.compileDeps(self.target);
    self.target_artifacts.demangle = self.compileDemangle(self.target);
    self.target_artifacts.support = try self.compileSupport(.{
        .platform = .target,
        .deps = self.target_artifacts.deps,
        .minimal = false,
    });

    for (enabled_targets) |target_name| {
        const incs = try self.buildTargetHeaders(target_name);
        for (incs) |inc| {
            self.target_artifacts.support.root_module.addIncludePath(inc.dirname());
        }
    }

    self.target_artifacts.target_parser_gen = self.runTargetParserGen();
    self.target_artifacts.target_parser = self.compileTargetParser();
    self.target_artifacts.bitstream_reader = self.compileBitstreamReader();
    self.target_artifacts.binary_format = self.compileBinaryFormat();
    self.target_artifacts.remarks = self.compileRemarks();
    self.target_artifacts.intrinsics_gen = self.runIntrinsicsGen();
    self.target_artifacts.core = self.compileCore();

    self.target_artifacts.bitcode.reader = self.compileBitcodeReader();
    self.target_artifacts.machine_code = self.compileMC();
    self.target_artifacts.asm_parser = self.compileAsmParser();
    self.target_artifacts.ir_reader = self.compileIRReader();
    self.target_artifacts.text_api.text_api = self.compileTextAPI();
    self.target_artifacts.object = self.compileObject();

    // The link test is completely ignored for packaging
    if (link_test_config) |link_test| {
        const llvm_link_test_exe = self.createLinkTest(link_test.cxx_flags);
        if (link_test.auto_install) b.installArtifact(llvm_link_test_exe);
    }
}

fn createHostModule(self: *const Self) *std.Build.Module {
    return self.b.createModule(.{
        .target = self.b.graph.host,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });
}

fn createTargetModule(self: *const Self) *std.Build.Module {
    return self.b.createModule(.{
        .target = self.target,
        .optimize = optimize,
        .link_libc = true,
        .link_libcpp = true,
    });
}

/// Creates a custom runnable target to test LLVM linkage against a C++23 source file
fn createLinkTest(self: *const Self, cxx_flags: []const []const u8) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createHostModule();
    mod.optimize = .Debug;

    mod.addCSourceFile(.{
        .file = b.path("packages/llvm/link_test.cpp"),
        .flags = cxx_flags,
    });
    mod.addSystemIncludePath(self.llvm.llvm_include);

    for (self.target_artifacts.artifactArray()) |lib| {
        mod.linkLibrary(lib);
    }

    const exe = b.addExecutable(.{
        .name = "llvm_link_test",
        .root_module = mod,
    });

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("link-test", "Run the llvm link test executable");
    run_step.dependOn(&run_cmd.step);

    return exe;
}

const ConfigHeaders = struct {
    config_h: *std.Build.Step.ConfigHeader,
    llvm_config_h: *std.Build.Step.ConfigHeader,
    abi_breaking_h: *std.Build.Step.ConfigHeader,
    targets_h: *std.Build.Step.ConfigHeader,
    asm_parsers_def: *std.Build.Step.ConfigHeader,
    asm_printers_def: *std.Build.Step.ConfigHeader,
    disassemblers_def: *std.Build.Step.ConfigHeader,
    target_exegesis_def: *std.Build.Step.ConfigHeader,
    target_mcas_def: *std.Build.Step.ConfigHeader,
    targets_def: *std.Build.Step.ConfigHeader,

    fn configHeaderArray(self: *const ConfigHeaders) [10]*std.Build.Step.ConfigHeader {
        return .{
            self.config_h,
            self.llvm_config_h,
            self.abi_breaking_h,
            self.targets_h,
            self.asm_parsers_def,
            self.asm_printers_def,
            self.disassemblers_def,
            self.target_exegesis_def,
            self.target_mcas_def,
            self.targets_def,
        };
    }
};

/// https://github.com/llvm/llvm-project/tree/llvmorg-21.1.8/llvm/include/llvm/Config
fn createConfigHeaders(self: *const Self, target: std.Target) !ConfigHeaders {
    const b = self.b;
    const is_darwin = target.os.tag.isDarwin();
    const is_windows = target.os.tag == .windows;
    const is_linux = target.os.tag == .linux;

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/config.h.cmake
    const config = b.addConfigHeader(.{
        .style = .{ .cmake = self.llvm.root.path(b, "llvm/include/llvm/Config/config.h.cmake") },
        .include_path = "llvm/Config/config.h",
    }, .{
        .PACKAGE_NAME = "LLVM",
        .PACKAGE_VERSION = version_str,
        .PACKAGE_STRING = "LLVM-conch-" ++ version_str,
        .PACKAGE_BUGREPORT = "https://github.com/llvm/llvm-project/issues/",
        .PACKAGE_VENDOR = "https://github.com/trevorswan11/conch",
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
        .BACKTRACE_HEADER = if (is_darwin or is_linux) "execinfo.h" else "link.h",
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

        // Host Intrinsics
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
        .LTDL_SHLIB_EXT = target.dynamicLibSuffix(),
        .LLVM_PLUGIN_EXT = target.dynamicLibSuffix(),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/llvm-config.h.cmake
    const triple = try target.linuxTriple(b.allocator);

    const llvm_config = b.addConfigHeader(.{
        .style = .{ .cmake = self.llvm.root.path(b, "llvm/include/llvm/Config/llvm-config.h.cmake") },
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
        .LLVM_NATIVE_ARCH = @tagName(target.cpu.arch),
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

        .LLVM_ENABLE_DEBUGLOC_TRACKING_COVERAGE = 0,
        .LLVM_ENABLE_DEBUGLOC_TRACKING_ORIGIN = 0,
        .LLVM_ENABLE_ONDISK_CAS = 0,
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/abi-breaking.h.cmake
    const abi_breaking = b.addConfigHeader(.{
        .style = .{ .cmake = self.llvm.root.path(b, "llvm/include/llvm/Config/abi-breaking.h.cmake") },
        .include_path = "llvm/Config/abi-breaking.h",
    }, .{
        .LLVM_ENABLE_ABI_BREAKING_CHECKS = 0,
        .LLVM_ENABLE_REVERSE_ITERATION = 0,
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/Targets.h.cmake
    const targets_h = b.addConfigHeader(.{
        .style = .{ .cmake = self.llvm.root.path(b, "llvm/include/llvm/Config/Targets.h.cmake") },
        .include_path = "llvm/Config/Targets.h",
    }, .{});

    // Dynamically fill every single target check
    inline for (enabled_targets) |enabled_target| {
        var macro_buf: [enabled_target.len]u8 = undefined;
        const macro_name = b.fmt("LLVM_HAS_{s}_TARGET", .{std.ascii.upperString(&macro_buf, enabled_target)});

        // Check if current target is in our enabled list
        const is_enabled: i32 = blk: for (enabled_targets) |enabled| {
            if (std.mem.eql(u8, enabled_target, enabled)) {
                break :blk 1;
            }
        } else 0;

        targets_h.addValue(macro_name, i32, is_enabled);
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/AsmParsers.def.in
    const asm_parsers = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/AsmParsers.def.in") },
        .include_path = "llvm/Config/AsmParsers.def",
    }, .{
        .LLVM_ENUM_ASM_PARSERS = formatDef(b, enabled_targets, "LLVM_ASM_PARSER"),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/AsmPrinters.def.in
    const asm_printers = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/AsmPrinters.def.in") },
        .include_path = "llvm/Config/AsmPrinters.def",
    }, .{
        .LLVM_ENUM_ASM_PRINTERS = formatDef(b, enabled_targets, "LLVM_ASM_PRINTER"),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/Disassemblers.def.in
    const disassemblers = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/Disassemblers.def.in") },
        .include_path = "llvm/Config/Disassemblers.def",
    }, .{
        .LLVM_ENUM_DISASSEMBLERS = formatDef(b, enabled_targets, "LLVM_DISASSEMBLER"),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/TargetExegesis.def.in
    const target_exegesis = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/TargetExegesis.def.in") },
        .include_path = "llvm/Config/TargetExegesis.def",
    }, .{
        .LLVM_ENUM_EXEGESIS = formatDef(b, enabled_targets, "LLVM_EXEGESIS"),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/TargetMCAs.def.in
    const target_mcas = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/TargetMCAs.def.in") },
        .include_path = "llvm/Config/TargetMCAs.def",
    }, .{
        .LLVM_ENUM_TARGETMCAS = formatDef(b, enabled_targets, "LLVM_TARGETMCA"),
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/Config/Targets.def.in
    const targets_def = b.addConfigHeader(.{
        .style = .{ .autoconf_at = self.llvm.root.path(b, "llvm/include/llvm/Config/Targets.def.in") },
        .include_path = "llvm/Config/Targets.def",
    }, .{
        .LLVM_ENUM_TARGETS = formatDef(b, enabled_targets, "LLVM_TARGET"),
    });

    return .{
        .config_h = config,
        .llvm_config_h = llvm_config,
        .abi_breaking_h = abi_breaking,
        .targets_h = targets_h,
        .asm_parsers_def = asm_parsers,
        .asm_printers_def = asm_printers,
        .disassemblers_def = disassemblers,
        .target_exegesis_def = target_exegesis,
        .target_mcas_def = target_mcas,
        .targets_def = targets_def,
    };
}

// Creates "macro_name(target1) macro_name(target2)... "
fn formatDef(b: *std.Build, targets: []const []const u8, macro_name: []const u8) []const u8 {
    var list: std.ArrayList(u8) = .empty;
    for (targets) |target| {
        list.print(b.allocator, "{s}({s}) ", .{ macro_name, target }) catch unreachable;
    }
    return list.items;
}

fn compileSupport(self: *const Self, config: struct {
    platform: Platform,
    deps: Dependencies,
    minimal: bool,
}) !*std.Build.Step.Compile {
    const b = self.b;
    const mod = switch (config.platform) {
        .host => self.createHostModule(),
        .target => self.createTargetModule(),
    };
    const target_result = mod.resolved_target.?.result;

    const support_root = self.llvm.root.path(b, support.common_root);
    mod.addCSourceFiles(.{
        .root = support_root,
        .files = &support.common_sources,
        .flags = &common_llvm_cxx_flags,
    });
    mod.addCSourceFiles(.{
        .root = support_root,
        .files = &support.regex_sources,
        .flags = &.{"-std=c11"},
    });

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, support.blake3_path),
        .files = &support.blake3_sources,
        .flags = &.{"-std=c11"},
    });

    const config_headers = try self.createConfigHeaders(target_result);
    const config_header_array = config_headers.configHeaderArray();
    for (config_header_array) |header| {
        mod.addConfigHeader(header);
    }

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.llvm.root.path(b, ThirdParty.siphash_inc));

    mod.linkLibrary(config.deps.zlib.artifact);
    mod.linkLibrary(config.deps.zstd.artifact);
    mod.linkLibrary(config.deps.libxml2.artifact);
    if (config.minimal) {
        mod.linkLibrary(self.minimal_artifacts.demangle);
    } else {
        mod.linkLibrary(self.target_artifacts.demangle);
    }

    // Specific windows compilation & linking
    if (target_result.os.tag == .windows) {
        mod.addCMacro("_CRT_SECURE_NO_DEPRECATE", "");
        mod.addCMacro("_CRT_SECURE_NO_WARNINGS", "");
        mod.addCMacro("_CRT_NONSTDC_NO_DEPRECATE", "");
        mod.addCMacro("_CRT_NONSTDC_NO_WARNINGS", "");
        mod.addCMacro("_SCL_SECURE_NO_WARNINGS", "");
        mod.addCMacro("UNICODE", "");
        mod.addCMacro("_UNICODE", "");

        mod.linkSystemLibrary("psapi", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("shell32", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("ole32", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("uuid", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("advapi32", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("ws2_32", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("ntdll", .{ .preferred_link_mode = .static });
    } else {
        if (target_result.os.tag == .linux) {
            mod.linkSystemLibrary("rt", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("dl", .{ .preferred_link_mode = .static });
        }

        mod.linkSystemLibrary("m", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("pthread", .{ .preferred_link_mode = .static });
    }

    const lib = b.addLibrary(.{
        .name = if (config.minimal) "LLVMSupportMinimal" else "LLVMSupport",
        .root_module = mod,
    });
    for (config_header_array) |header| {
        lib.installConfigHeader(header);
    }

    return lib;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Demangle/CMakeLists.txt
fn compileDemangle(self: *const Self, target: std.Build.ResolvedTarget) *std.Build.Step.Compile {
    const b = self.b;
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, "llvm/lib/Demangle"),
        .files = &.{
            "DLangDemangle.cpp",          "Demangle.cpp",
            "ItaniumDemangle.cpp",        "MicrosoftDemangle.cpp",
            "MicrosoftDemangleNodes.cpp", "RustDemangle.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });
    mod.addIncludePath(self.llvm.llvm_include);

    return b.addLibrary(.{
        .name = "LLVMDemangle",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/BinaryFormat/CMakeLists.txt
fn compileBinaryFormat(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    const bf_dir = self.llvm.root.path(b, "llvm/lib/BinaryFormat");
    mod.addCSourceFiles(.{
        .root = bf_dir,
        .files = &.{
            "AMDGPUMetadataVerifier.cpp", "COFF.cpp",
            "Dwarf.cpp",                  "DXContainer.cpp",
            "ELF.cpp",                    "MachO.cpp",
            "Magic.cpp",                  "Minidump.cpp",
            "MsgPackDocument.cpp",        "MsgPackDocumentYAML.cpp",
            "MsgPackReader.cpp",          "MsgPackWriter.cpp",
            "Wasm.cpp",                   "XCOFF.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);

    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.target_parser);

    return b.addLibrary(.{
        .name = "LLVMBinaryFormat",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Remarks/CMakeLists.txt
fn compileRemarks(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    const bf_dir = self.llvm.root.path(b, "llvm/lib/Remarks");
    mod.addCSourceFiles(.{
        .root = bf_dir,
        .files = &.{
            "BitstreamRemarkParser.cpp", "BitstreamRemarkSerializer.cpp",
            "Remark.cpp",                "RemarkFormat.cpp",
            "RemarkLinker.cpp",          "RemarkParser.cpp",
            "RemarkSerializer.cpp",      "RemarkStreamer.cpp",
            "RemarkStringTable.cpp",     "YAMLRemarkParser.cpp",
            "YAMLRemarkSerializer.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.minimal_artifacts.gen_vt.getDirectory());

    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.bitstream_reader);

    return b.addLibrary(.{
        .name = "LLVMRemarks",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Bitstream/Reader/CMakeLists.txt
fn compileBitstreamReader(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    mod.addCSourceFile(.{
        .file = self.llvm.root.path(b, "llvm/lib/Bitstream/Reader/BitstreamReader.cpp"),
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.linkLibrary(self.target_artifacts.support);

    return b.addLibrary(.{
        .name = "LLVMBitstreamReader",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/TargetParser/CMakeLists.txt
fn runTargetParserGen(self: *Self) *std.Build.Step.WriteFile {
    const b = self.b;
    const write_file = b.addWriteFiles();

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "AArch64TargetParserDef",
        .td_file = "llvm/lib/Target/AArch64/AArch64.td",
        .instruction = .{ .action = "-gen-arm-target-def" },
        .virtual_path = "llvm/TargetParser/AArch64TargetParserDef.inc",
        .extra_includes = &.{self.llvm.root.path(b, "llvm/lib/Target/AArch64")},
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "ARMTargetParserDef",
        .td_file = "llvm/lib/Target/ARM/ARM.td",
        .instruction = .{ .action = "-gen-arm-target-def" },
        .virtual_path = "llvm/TargetParser/ARMTargetParserDef.inc",
        .extra_includes = &.{self.llvm.root.path(b, "llvm/lib/Target/ARM")},
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "RISCVTargetParserDef",
        .td_file = "llvm/lib/Target/RISCV/RISCV.td",
        .instruction = .{ .action = "-gen-riscv-target-def" },
        .virtual_path = "llvm/TargetParser/RISCVTargetParserDef.inc",
        .extra_includes = &.{self.llvm.root.path(b, "llvm/lib/Target/RISCV")},
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "PPCGenTargetFeatures",
        .td_file = "llvm/lib/Target/PowerPC/PPC.td",
        .instruction = .{ .action = "-gen-target-features" },
        .virtual_path = "llvm/TargetParser/PPCGenTargetFeatures.inc",
        .extra_includes = &.{self.llvm.root.path(b, "llvm/lib/Target/PowerPC")},
    });

    return write_file;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TargetParser/CMakeLists.txt
fn compileTargetParser(self: *Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    const tp_dir = self.llvm.root.path(b, "llvm/lib/TargetParser");
    mod.addCSourceFiles(.{
        .root = tp_dir,
        .files = &.{
            "AArch64TargetParser.cpp", "ARMTargetParserCommon.cpp",
            "ARMTargetParser.cpp",     "CSKYTargetParser.cpp",
            "Host.cpp",                "LoongArchTargetParser.cpp",
            "PPCTargetParser.cpp",     "RISCVISAInfo.cpp",
            "RISCVTargetParser.cpp",   "SubtargetFeature.cpp",
            "TargetParser.cpp",        "Triple.cpp",
            "X86TargetParser.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.target_artifacts.target_parser_gen.getDirectory());

    mod.linkLibrary(self.target_artifacts.support);

    return b.addLibrary(.{
        .name = "LLVMTargetParser",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/include/llvm/IR/CMakeLists.txt
fn runIntrinsicsGen(self: *Self) *std.Build.Step.WriteFile {
    const b = self.b;
    const write_file = b.addWriteFiles();

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "Attributes",
        .td_file = "llvm/include/llvm/IR/Attributes.td",
        .instruction = .{ .action = "-gen-attrs" },
        .virtual_path = "llvm/IR/Attributes.inc",
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "IntrinsicEnums",
        .td_file = "llvm/include/llvm/IR/Intrinsics.td",
        .instruction = .{ .action = "-gen-intrinsic-enums" },
        .virtual_path = "llvm/IR/IntrinsicEnums.inc",
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "RuntimeLibcalls",
        .td_file = "llvm/include/llvm/IR/RuntimeLibcalls.td",
        .instruction = .{ .action = "-gen-runtime-libcalls" },
        .virtual_path = "llvm/IR/RuntimeLibcalls.inc",
    });

    _ = self.synthesizeHeader(write_file, .{
        .tblgen = self.full_tblgen,
        .name = "IntrinsicImpl",
        .td_file = "llvm/include/llvm/IR/Intrinsics.td",
        .instruction = .{ .action = "-gen-intrinsic-impl" },
        .virtual_path = "llvm/IR/IntrinsicImpl.inc",
    });

    for (ir.intrinsic_info) |info| {
        _ = self.synthesizeHeader(write_file, .{
            .tblgen = self.full_tblgen,
            .name = b.fmt("Intrinsics{s}", .{info.arch}),
            .td_file = "llvm/include/llvm/IR/Intrinsics.td",
            .instruction = .{
                .actions = &.{ "-gen-intrinsic-enums", b.fmt("-intrinsic-prefix={s}", .{info.prefix}) },
            },
            .virtual_path = b.fmt("llvm/IR/Intrinsics{s}.h", .{info.arch}),
        });
    }

    return write_file;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/IR/CMakeLists.txt
fn compileCore(self: *Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, ir.root),
        .files = &ir.sources,
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.minimal_artifacts.gen_vt.getDirectory());
    mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());

    mod.linkLibrary(self.target_artifacts.binary_format);
    mod.linkLibrary(self.target_artifacts.demangle);
    mod.linkLibrary(self.target_artifacts.remarks);
    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.target_parser);

    return b.addLibrary(.{
        .name = "LLVMCore",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Bitcode/Reader/CMakeLists.txt
fn compileBitcodeReader(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, "llvm/lib/Bitcode/Reader"),
        .files = &.{
            "BitcodeAnalyzer.cpp",
            "BitReader.cpp",
            "BitcodeReader.cpp",
            "MetadataLoader.cpp",
            "ValueList.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());

    mod.linkLibrary(self.target_artifacts.bitstream_reader);
    mod.linkLibrary(self.target_artifacts.core);
    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.target_parser);

    return b.addLibrary(.{
        .name = "LLVMBitReader",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/CMakeLists.txt
fn compileMC(self: *const Self) TargetArtifacts.MachineCode {
    const b = self.b;
    var mc: TargetArtifacts.MachineCode = .{};

    mc.mc = blk: {
        const mod = self.createTargetModule();
        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, machine_code.mc_root),
            .files = &machine_code.mc_sources,
            .flags = &common_llvm_cxx_flags,
        });

        mod.addIncludePath(self.llvm.llvm_include);
        mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());

        mod.linkLibrary(self.target_artifacts.support);
        mod.linkLibrary(self.target_artifacts.target_parser);
        mod.linkLibrary(self.target_artifacts.binary_format);

        break :blk b.addLibrary(.{
            .name = "LLVMMC",
            .root_module = mod,
        });
    };

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/MCDisassembler/CMakeLists.txt
    mc.disassembler = blk: {
        const mod = self.createTargetModule();
        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, machine_code.disassembler_root),
            .files = &machine_code.disassembler_sources,
            .flags = &common_llvm_cxx_flags,
        });

        mod.addIncludePath(self.llvm.llvm_include);
        mod.linkLibrary(self.target_artifacts.support);
        mod.linkLibrary(self.target_artifacts.target_parser);
        mod.linkLibrary(mc.mc);

        break :blk b.addLibrary(.{
            .name = "LLVMMCDisassembler",
            .root_module = mod,
        });
    };

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/MCParser/CMakeLists.txt
    mc.parser = blk: {
        const mod = self.createTargetModule();
        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, machine_code.parser_root),
            .files = &machine_code.parser_sources,
            .flags = &common_llvm_cxx_flags,
        });

        mod.addIncludePath(self.llvm.llvm_include);
        mod.linkLibrary(self.target_artifacts.support);
        mod.linkLibrary(self.target_artifacts.target_parser);
        mod.linkLibrary(mc.mc);

        break :blk b.addLibrary(.{
            .name = "LLVMMCParser",
            .root_module = mod,
        });
    };

    return mc;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/AsmParser/CMakeLists.txt
fn compileAsmParser(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, "llvm/lib/AsmParser"),
        .files = &.{
            "LLLexer.cpp",
            "LLParser.cpp",
            "Parser.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());

    mod.linkLibrary(self.target_artifacts.binary_format);
    mod.linkLibrary(self.target_artifacts.core);
    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.target_parser);

    return b.addLibrary(.{
        .name = "LLVMAsmParser",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/IRReader/CMakeLists.txt
fn compileIRReader(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();

    mod.addCSourceFile(.{
        .file = self.llvm.root.path(b, "llvm/lib/IRReader/IRReader.cpp"),
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());

    mod.linkLibrary(self.target_artifacts.asm_parser);
    mod.linkLibrary(self.target_artifacts.bitcode.reader);
    mod.linkLibrary(self.target_artifacts.core);
    mod.linkLibrary(self.target_artifacts.support);

    return b.addLibrary(.{
        .name = "LLVMIRReader",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TextAPI/CMakeLists.txt
fn compileTextAPI(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();
    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, "llvm/lib/TextAPI"),
        .files = &.{
            "Architecture.cpp",  "ArchitectureSet.cpp",
            "InterfaceFile.cpp", "TextStubV5.cpp",
            "PackedVersion.cpp", "Platform.cpp",
            "RecordsSlice.cpp",  "RecordVisitor.cpp",
            "Symbol.cpp",        "SymbolSet.cpp",
            "Target.cpp",        "TextAPIError.cpp",
            "TextStub.cpp",      "TextStubCommon.cpp",
            "Utils.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.binary_format);
    mod.linkLibrary(self.target_artifacts.target_parser);

    return b.addLibrary(.{
        .name = "LLVMTextAPI",
        .root_module = mod,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Object/CMakeLists.txt
fn compileObject(self: *const Self) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createTargetModule();
    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, object.root),
        .files = &object.sources,
        .flags = &common_llvm_cxx_flags,
    });

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.target_artifacts.intrinsics_gen.getDirectory());
    mod.addConfigHeader(self.llvm.vcs_revision);

    mod.linkLibrary(self.target_artifacts.bitcode.reader);
    mod.linkLibrary(self.target_artifacts.core);
    mod.linkLibrary(self.target_artifacts.machine_code.mc);
    mod.linkLibrary(self.target_artifacts.ir_reader);
    mod.linkLibrary(self.target_artifacts.binary_format);
    mod.linkLibrary(self.target_artifacts.machine_code.parser);
    mod.linkLibrary(self.target_artifacts.support);
    mod.linkLibrary(self.target_artifacts.target_parser);
    mod.linkLibrary(self.target_artifacts.text_api.text_api);

    return b.addLibrary(.{
        .name = "LLVMObject",
        .root_module = mod,
    });
}

/// Compiles tblgen (for the host system only)
fn compileTblgen(self: *const Self, config: struct {
    support_lib: *std.Build.Step.Compile,
    minimal: bool,
}) *std.Build.Step.Compile {
    const b = self.b;
    const mod = self.createHostModule();

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, tblgen.basic_root),
        .files = &tblgen.basic_sources,
        .flags = &common_llvm_cxx_flags,
    });

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, tblgen.lib_root),
        .files = &tblgen.lib_sources,
        .flags = &common_llvm_cxx_flags,
    });

    if (config.minimal) {
        mod.addCSourceFile(.{
            .file = self.llvm.root.path(b, tblgen.minimal_main),
            .flags = &common_llvm_cxx_flags,
        });
    } else {
        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, tblgen.common_root),
            .files = &tblgen.common,
            .flags = &common_llvm_cxx_flags,
        });

        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, tblgen.emitter_root),
            .files = &tblgen.emitter_sources,
            .flags = &common_llvm_cxx_flags,
        });
    }

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.llvm.root.path(b, "llvm/utils/TableGen"));

    const exe = b.addExecutable(.{
        .name = if (config.minimal) "LLVMTableGenMinimal" else "LLVMTableGen",
        .root_module = mod,
    });
    exe.linkLibrary(config.support_lib);

    return exe;
}

const TblgenInstruction = union(enum) {
    action: []const u8,
    actions: []const []const u8,
};

/// Uses tblgen to generate a build-time dependency for LLVM
/// - tg_tool: The tblgen binary to use
/// - name: The name of the generated .inc file
/// - td_file: The .td file, relative to LLVM root
/// - instruction: The tablegen action(s) to run, e.g. "-gen-intrinsic-enums"
/// - extra_includes: Optional includes to add to the include path
fn generateTblgenInc(self: *const Self, config: struct {
    tblgen: *std.Build.Step.Compile,
    name: []const u8,
    td_file: []const u8,
    instruction: TblgenInstruction,
    extra_includes: ?[]const std.Build.LazyPath = null,
}) std.Build.LazyPath {
    const b = self.b;
    const run = b.addRunArtifact(config.tblgen);
    run.step.name = b.fmt("TableGen {s}", .{config.name});

    switch (config.instruction) {
        .action => |action| run.addArg(action),
        .actions => |action| run.addArgs(action),
    }

    run.addPrefixedDirectoryArg("-I", self.llvm.llvm_include);
    if (config.extra_includes) |extra_includes| {
        for (extra_includes) |include| {
            run.addPrefixedDirectoryArg("-I", include);
        }
    }

    run.addFileArg(self.llvm.root.path(b, config.td_file));
    run.addArg("-o");

    return run.addOutputFileArg(b.fmt("{s}.inc", .{config.name}));
}

/// Generate the passed td file as the requested inc, adding it to the virtual path registry.
fn synthesizeHeader(self: *Self, registry: *std.Build.Step.WriteFile, config: struct {
    tblgen: *std.Build.Step.Compile,
    name: []const u8,
    td_file: []const u8,
    instruction: TblgenInstruction,
    virtual_path: []const u8,
    extra_includes: ?[]const std.Build.LazyPath = null,
}) std.Build.LazyPath {
    const flat = self.generateTblgenInc(.{
        .tblgen = config.tblgen,
        .name = config.name,
        .td_file = config.td_file,
        .instruction = config.instruction,
        .extra_includes = config.extra_includes,
    });
    return registry.addCopyFile(flat, config.virtual_path);
}

/// Generates all .inc files for a specific target
fn buildTargetHeaders(self: *const Self, target_name: []const u8) ![]std.Build.LazyPath {
    const b = self.b;
    var results: std.ArrayList(std.Build.LazyPath) = .empty;

    // The root .td file for a target is always <Name>.td
    const target_dir = b.fmt("llvm/lib/Target/{s}", .{target_name});
    const main_td = b.fmt("{s}/{s}.td", .{ target_dir, targetAbbr(target_name) });
    for (tblgen.actions) |action| {
        const name = b.fmt("{s}{s}", .{ target_name, action.name });
        try results.append(
            b.allocator,
            self.generateTblgenInc(.{
                .tblgen = self.full_tblgen,
                .name = name,
                .td_file = main_td,
                .instruction = .{ .action = action.flag },
                .extra_includes = &.{self.llvm.root.path(b, target_dir)},
            }),
        );
    }

    return results.items;
}

fn compileDeps(self: *const Self, target: std.Build.ResolvedTarget) !Dependencies {
    const zlib_dep = try self.compileZlib(target);
    const zlib_include = zlib_dep.dependency.path(".");

    const libxml2_dep = self.compileLibxml2(.{
        .target = target,
        .zlib_include = zlib_include,
    });

    const zstd_dep = self.compileZstd(.{
        .target = target,
        .zlib_include = zlib_include,
    });

    return .{
        .zlib = zlib_dep,
        .libxml2 = libxml2_dep,
        .zstd = zstd_dep,
    };
}

/// Compiles zlib from source as a static library
/// Reference: https://github.com/allyourcodebase/zlib
fn compileZlib(self: *const Self, target: std.Build.ResolvedTarget) !Dependencies.Dependency {
    const b = self.b;
    const upstream = b.dependency("zlib", .{});
    const root = upstream.path(".");
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
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
    if (target.result.os.tag != .windows) {
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
    lib.installHeadersDirectory(upstream.path(""), "", .{
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
fn compileLibxml2(self: *const Self, config: struct {
    target: std.Build.ResolvedTarget,
    zlib_include: std.Build.LazyPath,
}) Dependencies.Dependency {
    const b = self.b;
    const upstream = b.dependency("libxml2", .{});
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = optimize,
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
        .LIBXML_VERSION_EXTRA = "-conch",
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
fn compileZstd(self: *const Self, config: struct {
    target: std.Build.ResolvedTarget,
    zlib_include: std.Build.LazyPath,
}) Dependencies.Dependency {
    const b = self.b;
    const upstream = b.dependency("zstd", .{});
    const lib_path = upstream.path("lib");

    const mod = b.createModule(.{
        .target = config.target,
        .optimize = optimize,
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
    lib.installHeader(lib_path.path(b, "zstd.h"), "zstd.h");
    lib.installHeader(lib_path.path(b, "zdict.h"), "zdict.h");
    lib.installHeader(lib_path.path(b, "zstd_errors.h"), "zstd_errors.h");

    return .{
        .dependency = upstream,
        .artifact = lib,
    };
}
