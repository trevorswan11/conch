//! LLVM Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

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

const common_llvm_cxx_flags = [_][]const u8{
    "-std=c++17",
    "-fno-exceptions",
    "-fno-rtti",
};

const Self = @This();

b: *std.Build,
llvm_dep: *std.Build.Dependency,
llvm_root: std.Build.LazyPath,
llvm_include: std.Build.LazyPath,

host_tablegen: *std.Build.Step.Compile = undefined,
host_deps: Dependencies = undefined,
host_support: *std.Build.Step.Compile = undefined,
target_deps: Dependencies = undefined,
target_support: *std.Build.Step.Compile = undefined,

pub fn build(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    auto_install: bool,
}) !Self {
    const upstream = b.dependency("llvm", .{});
    var llvm: Self = .{
        .b = b,
        .llvm_dep = upstream,
        .llvm_root = upstream.path("."),
        .llvm_include = upstream.path("llvm/include"),
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
        .auto_install = false,
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
        .auto_install = config.auto_install,
    });

    return llvm;
}

/// Need More, see https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/Config/Targets.h.cmake
/// and friends
fn createConfigHeaders(
    self: *const Self,
    target: std.Build.ResolvedTarget,
) !struct {
    config_h: *std.Build.Step.ConfigHeader,
    llvm_config_h: *std.Build.Step.ConfigHeader,
    abi_breaking_h: *std.Build.Step.ConfigHeader,
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
    const config = b.addConfigHeader(.{
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
    const llvm_config = b.addConfigHeader(.{
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

        .LLVM_ENABLE_DEBUGLOC_TRACKING_COVERAGE = 0,
        .LLVM_ENABLE_DEBUGLOC_TRACKING_ORIGIN = 0,
        .LLVM_ENABLE_ONDISK_CAS = 0,
    });

    // https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/Config/abi-breaking.h.cmake
    const abi_breaking = b.addConfigHeader(.{
        .style = .{ .cmake = self.llvm_root.path(b, "llvm/include/llvm/Config/abi-breaking.h.cmake") },
        .include_path = "llvm/Config/abi-breaking.h",
    }, .{
        .LLVM_ENABLE_ABI_BREAKING_CHECKS = 0,
        .LLVM_ENABLE_REVERSE_ITERATION = 0,
    });

    return .{
        .config_h = config,
        .llvm_config_h = llvm_config,
        .abi_breaking_h = abi_breaking,
    };
}

fn support(self: *const Self, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    deps: Dependencies,
    auto_install: bool,
}) !*std.Build.Step.Compile {
    const b = self.b;
    const t = config.target.result;

    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    const demangle_lib = self.demangle(.{
        .target = config.target,
        .optimize = config.optimize,
        .auto_install = config.auto_install,
    });
    mod.linkLibrary(demangle_lib);

    const config_headers = try self.createConfigHeaders(config.target);
    const support_root = self.llvm_root.path(b, SupportSources.common_path);
    mod.addIncludePath(config_headers.config_h.getOutputDir());
    mod.addIncludePath(config_headers.llvm_config_h.getOutputDir());
    mod.addIncludePath(config_headers.abi_breaking_h.getOutputDir());
    mod.addIncludePath(self.llvm_include);
    mod.addIncludePath(support_root);
    mod.addIncludePath(self.llvm_root.path(b, ThirdParty.siphash_inc));

    // Compile sources
    mod.addCSourceFiles(.{
        .root = support_root,
        .files = &SupportSources.regex_c,
        .flags = &.{"-std=c11"},
    });
    mod.addCSourceFiles(.{
        .root = support_root.path(b, SupportSources.blake3_rel_path),
        .files = &SupportSources.blake3,
        .flags = &.{"-std=c11"},
    });

    mod.addCSourceFiles(.{
        .root = support_root,
        .files = &SupportSources.common,
        .flags = &common_llvm_cxx_flags,
    });
    mod.addCSourceFiles(.{
        .root = support_root,
        .files = &SupportSources.common,
        .flags = &common_llvm_cxx_flags,
    });

    mod.linkLibrary(config.deps.zlib.artifact);
    mod.linkLibrary(config.deps.zstd.artifact);
    mod.linkLibrary(config.deps.libxml2.artifact);

    // Specific windows compilation & linking
    if (t.os.tag == .windows) {
        mod.addIncludePath(self.llvm_root.path(b, SupportSources.windows_rel_path));
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
        mod.addIncludePath(self.llvm_root.path(b, SupportSources.unix_rel_path));
        if (t.os.tag == .linux) {
            mod.linkSystemLibrary("rt", .{ .preferred_link_mode = .static });
            mod.linkSystemLibrary("dl", .{ .preferred_link_mode = .static });
        }

        mod.linkSystemLibrary("m", .{ .preferred_link_mode = .static });
        mod.linkSystemLibrary("pthread", .{ .preferred_link_mode = .static });
    }

    const lib = b.addLibrary(.{
        .name = "LLVMSupport",
        .root_module = mod,
    });

    lib.installConfigHeader(config_headers.config_h);
    lib.installConfigHeader(config_headers.llvm_config_h);
    lib.installConfigHeader(config_headers.abi_breaking_h);

    if (config.auto_install) b.installArtifact(lib);
    return lib;
}

fn demangle(self: *const Self, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    auto_install: bool,
}) *std.Build.Step.Compile {
    const b = self.b;
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    mod.addCSourceFiles(.{
        .root = self.llvm_root.path(b, "llvm/lib/demangle"),
        .files = &.{
            "DLangDemangle.cpp",
            "Demangle.cpp",
            "ItaniumDemangle.cpp",
            "MicrosoftDemangle.cpp",
            "MicrosoftDemangleNodes.cpp",
            "RustDemangle.cpp",
        },
        .flags = &common_llvm_cxx_flags,
    });
    mod.addIncludePath(self.llvm_include);

    const lib = b.addLibrary(.{
        .name = "LLVMDemangle",
        .root_module = mod,
    });
    if (config.auto_install) b.installArtifact(lib);
    return lib;
}

fn tablegen(
    self: *const Self,
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
        .root = self.llvm_root.path(b, TableGenSources.emitter_root),
        .files = &TableGenSources.emitters,
        .flags = &cpp_flags,
    });
    mod.addCSourceFiles(.{
        .root = self.llvm_root.path(b, TableGenSources.basic_root),
        .files = &TableGenSources.basic,
        .flags = &cpp_flags,
    });
    mod.addCSourceFiles(.{
        .root = self.llvm_root.path(b, TableGenSources.common_root),
        .files = &TableGenSources.common,
        .flags = &cpp_flags,
    });

    mod.addIncludePath(self.llvm_root.path(b, "llvm/include"));
    mod.addIncludePath(self.llvm_root.path(b, "llvm/utils/TableGen"));

    const exe = b.addExecutable(.{
        .name = "llvm_tablegen",
        .root_module = mod,
    });
    exe.linkLibrary(support_lib);

    return exe;
}

pub fn dependencies(self: *const Self, config: struct {
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
fn zlib(self: *const Self, config: struct {
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
fn libxml2(self: *const Self, config: struct {
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
fn zstd(self: *const Self, config: struct {
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
    lib.installHeader(lib_path.path(b, "zstd.h"), "zstd.h");
    lib.installHeader(lib_path.path(b, "zdict.h"), "zdict.h");
    lib.installHeader(lib_path.path(b, "zstd_errors.h"), "zstd_errors.h");

    return .{
        .dependency = upstream,
        .artifact = lib,
    };
}

const ThirdParty = struct {
    const siphash_inc = "third-party/siphash/include";
};

const TableGenSources = struct {
    // https://github.com/llvm/llvm-project/blob/main/llvm/lib/TableGen/CMakeLists.txt
    const basic_root = "llvm/lib/TableGen/";
    const basic = [_][]const u8{
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

    // https://github.com/llvm/llvm-project/blob/main/llvm/utils/TableGen/Common/CMakeLists.txt
    const common_root = emitter_root ++ "Common/";
    const common = [_][]const u8{
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
        "InfoByHwMode.cpp",
        "InstructionEncoding.cpp",
        "OptEmitter.cpp",
        "PredicateExpander.cpp",
        "SubtargetFeatureInfo.cpp",
        "Types.cpp",
        "Utils.cpp",
        "VarLenCodeEmitterGen.cpp",
    };

    // https://github.com/llvm/llvm-project/blob/main/llvm/utils/TableGen/CMakeLists.txt
    const emitter_root = "llvm/utils/TableGen/";
    const emitters = [_][]const u8{
        "AsmMatcherEmitter.cpp",            "AsmWriterEmitter.cpp",
        "CallingConvEmitter.cpp",           "CodeEmitterGen.cpp",
        "CodeGenMapTable.cpp",              "CompressInstEmitter.cpp",
        "CTagsEmitter.cpp",                 "DAGISelEmitter.cpp",
        "DAGISelMatcher.cpp",               "DAGISelMatcherEmitter.cpp",
        "DAGISelMatcherGen.cpp",            "DAGISelMatcherOpt.cpp",
        "DecoderEmitter.cpp",               "DecoderTableEmitter.cpp",
        "DecoderTree.cpp",                  "DFAEmitter.cpp",
        "DFAPacketizerEmitter.cpp",         "DisassemblerEmitter.cpp",
        "DXILEmitter.cpp",                  "ExegesisEmitter.cpp",
        "FastISelEmitter.cpp",              "GlobalISelCombinerEmitter.cpp",
        "GlobalISelEmitter.cpp",            "InstrDocsEmitter.cpp",
        "InstrInfoEmitter.cpp",             "llvm-tblgen.cpp",
        "MacroFusionPredicatorEmitter.cpp", "OptionParserEmitter.cpp",
        "OptionRSTEmitter.cpp",             "PseudoLoweringEmitter.cpp",
        "RegisterBankEmitter.cpp",          "RegisterInfoEmitter.cpp",
        "SDNodeInfoEmitter.cpp",            "SearchableTableEmitter.cpp",
        "SubtargetEmitter.cpp",             "WebAssemblyDisassemblerEmitter.cpp",
        "X86InstrMappingEmitter.cpp",       "X86DisassemblerTables.cpp",
        "X86FoldTablesEmitter.cpp",         "X86MnemonicTables.cpp",
        "X86ModRMFilters.cpp",              "X86RecognizableInstr.cpp",
    };
};

/// https://github.com/llvm/llvm-project/blob/main/llvm/lib/Support/CMakeLists.txt
const SupportSources = struct {
    const common_path = "llvm/lib/Support/";
    const common = [_][]const u8{
        "ABIBreak.cpp",               "AMDGPUMetadata.cpp",
        "APFixedPoint.cpp",           "APFloat.cpp",
        "APInt.cpp",                  "APSInt.cpp",
        "ARMBuildAttributes.cpp",     "AArch64AttributeParser.cpp",
        "AArch64BuildAttributes.cpp", "ARMAttributeParser.cpp",
        "ARMWinEH.cpp",               "Allocator.cpp",
        "AutoConvert.cpp",            "Base64.cpp",
        "BalancedPartitioning.cpp",   "BinaryStreamError.cpp",
        "BinaryStreamReader.cpp",     "BinaryStreamRef.cpp",
        "BinaryStreamWriter.cpp",     "BlockFrequency.cpp",
        "BranchProbability.cpp",      "BuryPointer.cpp",
        "CachePruning.cpp",           "Caching.cpp",
        "circular_raw_ostream.cpp",   "Chrono.cpp",
        "COM.cpp",                    "CodeGenCoverage.cpp",
        "CommandLine.cpp",            "Compression.cpp",
        "CRC.cpp",                    "ConvertUTF.cpp",
        "ConvertEBCDIC.cpp",          "ConvertUTFWrapper.cpp",
        "CrashRecoveryContext.cpp",   "CSKYAttributes.cpp",
        "CSKYAttributeParser.cpp",    "DataExtractor.cpp",
        "Debug.cpp",                  "DebugCounter.cpp",
        "DeltaAlgorithm.cpp",         "DeltaTree.cpp",
        "DivisionByConstantInfo.cpp", "DAGDeltaAlgorithm.cpp",
        "DJB.cpp",                    "DynamicAPInt.cpp",
        "ELFAttributes.cpp",          "ELFAttrParserCompact.cpp",
        "ELFAttrParserExtended.cpp",  "Error.cpp",
        "ErrorHandling.cpp",          "ExponentialBackoff.cpp",
        "ExtensibleRTTI.cpp",         "FileCollector.cpp",
        "FileUtilities.cpp",          "FileOutputBuffer.cpp",
        "FloatingPointMode.cpp",      "FoldingSet.cpp",
        "FormattedStream.cpp",        "FormatVariadic.cpp",
        "GlobPattern.cpp",            "GraphWriter.cpp",
        "HexagonAttributeParser.cpp", "HexagonAttributes.cpp",
        "InitLLVM.cpp",               "InstructionCost.cpp",
        "IntEqClasses.cpp",           "IntervalMap.cpp",
        "JSON.cpp",                   "KnownBits.cpp",
        "KnownFPClass.cpp",           "LEB128.cpp",
        "LineIterator.cpp",           "Locale.cpp",
        "LockFileManager.cpp",        "ManagedStatic.cpp",
        "MathExtras.cpp",             "MemAlloc.cpp",
        "MemoryBuffer.cpp",           "MemoryBufferRef.cpp",
        "ModRef.cpp",                 "MD5.cpp",
        "MSP430Attributes.cpp",       "MSP430AttributeParser.cpp",
        "Mustache.cpp",               "NativeFormatting.cpp",
        "OptimizedStructLayout.cpp",  "Optional.cpp",
        "OptionStrCmp.cpp",           "PGOOptions.cpp",
        "Parallel.cpp",               "PluginLoader.cpp",
        "PrettyStackTrace.cpp",       "RandomNumberGenerator.cpp",
        "Regex.cpp",                  "RewriteBuffer.cpp",
        "RewriteRope.cpp",            "RISCVAttributes.cpp",
        "RISCVAttributeParser.cpp",   "RISCVISAUtils.cpp",
        "ScaledNumber.cpp",           "ScopedPrinter.cpp",
        "SHA1.cpp",                   "SHA256.cpp",
        "Signposts.cpp",              "SipHash.cpp",
        "SlowDynamicAPInt.cpp",       "SmallPtrSet.cpp",
        "SmallVector.cpp",            "SourceMgr.cpp",
        "SpecialCaseList.cpp",        "Statistic.cpp",
        "StringExtras.cpp",           "StringMap.cpp",
        "StringSaver.cpp",            "StringRef.cpp",
        "SuffixTreeNode.cpp",         "SuffixTree.cpp",
        "SystemUtils.cpp",            "TarWriter.cpp",
        "TextEncoding.cpp",           "ThreadPool.cpp",
        "TimeProfiler.cpp",           "Timer.cpp",
        "ToolOutputFile.cpp",         "TrieRawHashMap.cpp",
        "Twine.cpp",                  "TypeSize.cpp",
        "Unicode.cpp",                "UnicodeCaseFold.cpp",
        "UnicodeNameToCodepoint.cpp", "UnicodeNameToCodepointGenerated.cpp",
        "VersionTuple.cpp",           "VirtualFileSystem.cpp",
        "WithColor.cpp",              "YAMLParser.cpp",
        "YAMLTraits.cpp",             "raw_os_ostream.cpp",
        "raw_ostream.cpp",            "raw_socket_stream.cpp",
        "xxhash.cpp",                 "Z3Solver.cpp",

        // System
        "Atomic.cpp",                 "DynamicLibrary.cpp",
        "Errno.cpp",                  "Memory.cpp",
        "Path.cpp",                   "Process.cpp",
        "Program.cpp",                "ProgramStack.cpp",
        "RWMutex.cpp",                "Signals.cpp",
        "Threading.cpp",              "Valgrind.cpp",
        "Watchdog.cpp",
    };

    const regex_c = [_][]const u8{
        "regcomp.c",
        "regerror.c",
        "regexec.c",
        "regfree.c",
        "regstrlcpy.c",
    };

    const blake3_rel_path = "BLAKE3";
    const blake3 = [_][]const u8{
        "blake3.c",
        "blake3_dispatch.c",
        "blake3_portable.c",
    };

    const unix_rel_path = "Unix";
    const unix_specific = [_][]const u8{
        "COM.inc",
        "DynamicLibrary.inc",
        "Jobserver.inc",
        "Memory.inc",
        "Path.inc",
        "Process.inc",
        "Program.inc",
        "README.txt",
        "Signals.inc",
        "Threading.inc",
        "Unix.h",
        "Watchdog.inc",
    };

    const windows_rel_path = "Windows";
    const windows_specific = [_][]const u8{
        "COM.inc",
        "DynamicLibrary.inc",
        "Jobserver.inc",
        "Memory.inc",
        "Path.inc",
        "Process.inc",
        "Program.inc",
        "Signals.inc",
        "Threading.inc",
        "Watchdog.inc",
        "explicit_symbols.inc",
    };
};
