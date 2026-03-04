//! Clang Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

const LLVMBuilder = @import("LLVMBuilder.zig");

const basic = @import("sources/clang/basic.zig");
const format = @import("sources/clang/format.zig");
const lex = @import("sources/clang/lex.zig");
const sema = @import("sources/clang/sema.zig");
const tblgen = @import("sources/clang/tblgen.zig");

const Artifact = LLVMBuilder.Artifact;
const ArtifactCreateConfig = LLVMBuilder.ArtifactCreateConfig;

const Metadata = struct {
    root: std.Build.LazyPath,
    clang_include: std.Build.LazyPath,
    vcs_version: *std.Build.Step.ConfigHeader,
};

/// Artifacts compiled for the actual target, associated with the clang subproject of llvm
const ClangTargetArtifacts = struct {
    const Driver = struct {
        options: *std.Build.Step.WriteFile = undefined,
        core_lib: Artifact = undefined,
    };

    const Basic = struct {
        gen: *std.Build.Step.WriteFile = undefined,
        core_lib: Artifact = undefined,
        version_inc: *std.Build.Step.ConfigHeader = undefined,
    };

    const Sema = struct {
        gen: *std.Build.Step.WriteFile = undefined,
        core_lib: Artifact = undefined,
    };

    const Tooling = struct {
        core: Artifact = undefined,
        inclusions: Artifact = undefined,
    };

    support: Artifact = undefined,
    driver: Driver = .{},
    sema: Sema = .{},
    basic: Basic = .{},
    lex: Artifact = undefined,
    rewrite: Artifact = undefined,
    tooling: Tooling = .{},

    format: Artifact = undefined,
};

/// CLI Tools provided by Clang
const ClangTools = struct {
    clang_format: Artifact = undefined,
};

const default_optimize = LLVMBuilder.default_optimize;
const common_llvm_cxx_flags = LLVMBuilder.common_llvm_cxx_flags;

const Self = @This();

b: *std.Build,
llvm: *const LLVMBuilder,
metadata: Metadata,

config_h: *std.Build.Step.ConfigHeader = undefined,
clang_artifacts: ClangTargetArtifacts = .{},
clang_tblgen: Artifact = undefined,
clang_tools: ClangTools = .{},

/// This is only true once `build` is called and exits successfully
complete: bool = false,

/// Creates a new clang builder from a potentially unfinished LLVM
pub fn init(llvm: *const LLVMBuilder) *Self {
    const b = llvm.b;
    const self = b.allocator.create(Self) catch @panic("OOM");

    self.* = .{
        .b = b,
        .llvm = llvm,
        .metadata = .{
            .root = llvm.metadata.root.dupe(b),
            .clang_include = llvm.metadata.root.path(b, "clang/include"),
            .vcs_version = b.addConfigHeader(.{ .include_path = "VCSVersion.inc" }, .{
                .LLVM_REVISION = LLVMBuilder.llvm_revision,
                .CLANG_REVISION = LLVMBuilder.llvm_revision,
                .LLVM_REPOSITORY = "https://github.com/llvm/llvm-project.git",
                .CLANG_REPOSITORY = "https://github.com/llvm/llvm-project.git",
            }),
        },
    };
    return self;
}

/// This must be called once llvm has been built with its `build`
pub fn build(self: *Self) void {
    if (!self.llvm.complete) {
        @panic("Misconfigured build script - LLVM was not built");
    }

    self.clang_artifacts.support = self.buildSupport();
    self.clang_tblgen = self.buildTblgen();
    self.config_h = self.createConfigH();

    self.clang_artifacts.sema.gen = self.runSemaGen();
    self.clang_artifacts.basic.gen = self.runBasicGen();
    self.clang_artifacts.basic.core_lib = self.buildBasic();
    self.clang_artifacts.lex = self.buildLex();
    self.clang_artifacts.rewrite = self.buildRewrite();
    self.clang_artifacts.tooling = self.buildTooling();
    self.clang_artifacts.format = self.buildFormat();

    self.clang_artifacts.driver.options = self.runDriverOptsGen();

    self.clang_tools.clang_format = self.buildFormatTool();

    self.complete = true;
}

/// Creates a module and corresponding library, defaulting to the target platform.
/// `llvm/include` and `clang/include` are implicitly included here
pub fn createClangLibrary(self: *const Self, config: ArtifactCreateConfig) Artifact {
    const lib = self.llvm.createLLVMLibrary(config);
    lib.root_module.addIncludePath(self.metadata.clang_include);
    return lib;
}

/// Creates a module and corresponding executable, defaulting to the target platform.
/// `llvm/include` and `clang/include` are implicitly included here
pub fn createClangExecutable(self: *const Self, config: ArtifactCreateConfig) Artifact {
    const exe = self.llvm.createLLVMExecutable(config);
    exe.root_module.addIncludePath(self.metadata.clang_include);
    return exe;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/include/clang/Config/config.h.cmake
fn createConfigH(self: *const Self) *std.Build.Step.ConfigHeader {
    const b = self.b;
    const is_windows = self.llvm.target.result.os.tag == .windows;

    return b.addConfigHeader(
        .{
            .style = .{ .cmake = self.metadata.clang_include.path(b, "clang/Config/config.h.cmake") },
            .include_path = "clang/Config/config.h",
        },
        .{
            .BUG_REPORT_URL = "https://github.com/llvm/llvm-project/issues",
            .CLANG_DEFAULT_PIE_ON_LINUX = 0,
            .CLANG_DEFAULT_LINKER = "",
            .CLANG_DEFAULT_CXX_STDLIB = "",
            .CLANG_DEFAULT_RTLIB = "",
            .CLANG_DEFAULT_UNWINDLIB = "",
            .CLANG_DEFAULT_OBJCOPY = "objcopy",
            .CLANG_DEFAULT_OPENMP_RUNTIME = "libomp",
            .CLANG_SYSTEMZ_DEFAULT_ARCH = "z10",
            .CLANG_INSTALL_LIBDIR_BASENAME = "lib",
            .CLANG_RESOURCE_DIR = "",
            .C_INCLUDE_DIRS = "",
            .CLANG_CONFIG_FILE_SYSTEM_DIR = null,
            .CLANG_CONFIG_FILE_USER_DIR = null,
            .DEFAULT_SYSROOT = "",
            .GCC_INSTALL_PREFIX = "",
            .CLANG_HAVE_LIBXML = 1,
            .CLANG_HAVE_RLIMITS = @intFromBool(!is_windows),
            .CLANG_HAVE_DLFCN_H = @intFromBool(!is_windows),
            .CLANG_HAVE_DLADDR = @intFromBool(!is_windows),
            .HOST_LINK_VERSION = null,
            .ENABLE_LINKER_BUILD_ID = null,
            .ENABLE_X86_RELAX_RELOCATIONS = 0,
            .PPC_LINUX_DEFAULT_IEEELONGDOUBLE = 0,
            .CLANG_ENABLE_OBJC_REWRITER = 1,
            .CLANG_ENABLE_STATIC_ANALYZER = 1,
            .CLANG_SPAWN_CC1 = 0,
            .CLANG_ENABLE_CIR = 0,
        },
    );
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Support/CMakeLists.txt
fn buildSupport(self: *const Self) Artifact {
    const b = self.b;
    return self.createClangLibrary(.{
        .name = "clangSupport",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, "clang/lib/Support"),
            .files = &.{"RISCVVIntrinsicUtils.cpp"},
        },
        .link_libraries = &.{self.llvm.target_artifacts.support},
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/utils/TableGen/CMakeLists.txt
fn buildTblgen(self: *const Self) Artifact {
    const b = self.b;
    return self.createClangExecutable(.{
        .name = "ClangTableGen",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, tblgen.root),
            .files = &tblgen.sources,
        },
        .link_libraries = &.{
            self.llvm.target_artifacts.support,
            self.llvm.configure_phase_artifacts.lib_full_tblgen,
            self.clang_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/include/clang/Sema/CMakeLists.txt
fn runSemaGen(self: *const Self) *std.Build.Step.WriteFile {
    const b = self.b;
    const registry = b.addWriteFiles();

    // Diagnostics need direct include access to Basic
    for (sema.attr_synthesize_configs) |config| {
        var adj = config;
        adj.gen_conf.tblgen = self.clang_tblgen;
        adj.gen_conf.extra_includes = &.{self.metadata.clang_include};
        self.llvm.synthesizeHeader(registry, adj);
    }

    return registry;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/include/clang/Basic/CMakeLists.txt
fn runBasicGen(self: *const Self) *std.Build.Step.WriteFile {
    const b = self.b;
    const registry = b.addWriteFiles();

    // Diagnostics need direct include access to Basic
    for (basic.diag_synthesize_configs) |config| {
        var adj = config;
        adj.gen_conf.tblgen = self.clang_tblgen;
        adj.gen_conf.extra_includes = &.{
            self.metadata.root.path(b, basic.include_root),
        };
        self.llvm.synthesizeHeader(registry, adj);
    }

    // Attributes need clang include
    for (basic.attr_synthesize_configs) |config| {
        var adj = config;
        adj.gen_conf.tblgen = self.clang_tblgen;
        adj.gen_conf.extra_includes = &.{self.metadata.clang_include};
        self.llvm.synthesizeHeader(registry, adj);
    }

    // Builtins need clang include
    for (basic.builtin_synthesize_configs) |config| {
        var adj = config;
        adj.gen_conf.tblgen = self.clang_tblgen;
        adj.gen_conf.extra_includes = &.{self.metadata.clang_include};
        self.llvm.synthesizeHeader(registry, adj);
    }

    // Arch specific need direct include access to Basic
    for (basic.arch_synthesize_configs) |config| {
        var adj = config;
        adj.gen_conf.tblgen = self.clang_tblgen;
        adj.gen_conf.extra_includes = &.{
            self.metadata.root.path(b, basic.include_root),
        };
        self.llvm.synthesizeHeader(registry, adj);
    }

    return registry;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/include/clang/Driver/CMakeLists.txt
fn runDriverOptsGen(self: *const Self) *std.Build.Step.WriteFile {
    const opts = self.b.addWriteFiles();

    // This uses LLVM's tablegen for some reason
    self.llvm.synthesizeHeader(opts, .{
        .gen_conf = .{
            .name = "Options",
            .td_file = "clang/include/clang/Driver/Options.td",
            .instruction = .{ .action = "-gen-opt-parser-defs" },
        },
        .virtual_path = "clang/Driver/Options.inc",
    });
    return opts;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Basic/CMakeLists.txt
fn buildBasic(self: *Self) Artifact {
    const b = self.b;
    const llvm = self.llvm;
    self.clang_artifacts.basic.version_inc = b.addConfigHeader(.{
        .style = .{
            .cmake = self.metadata.root.path(b, basic.include_root ++ "Version.inc.in"),
        },
        .include_path = "clang/Basic/Version.inc",
    }, .{
        .CLANG_VERSION = LLVMBuilder.version_str,
        .CLANG_VERSION_STRING = LLVMBuilder.version_str,
        .CLANG_VERSION_MAJOR = @as(i32, @intCast(LLVMBuilder.version.major)),
        .CLANG_VERSION_MAJOR_STRING = std.fmt.comptimePrint(
            "{d}",
            .{LLVMBuilder.version.major},
        ),
        .CLANG_VERSION_MINOR = @as(i32, @intCast(LLVMBuilder.version.minor)),
        .CLANG_VERSION_PATCHLEVEL = @as(i32, @intCast(LLVMBuilder.version.patch)),
        .MAX_CLANG_ABI_COMPAT_VERSION = @as(i32, @intCast(LLVMBuilder.version.major)),
    });

    return self.createClangLibrary(.{
        .name = "clangBasic",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, basic.root),
            .files = &basic.sources,
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
            self.clang_artifacts.sema.gen.getDirectory(),
            llvm.target_artifacts.target_backends.parser_gen.getDirectory(),
            llvm.target_artifacts.frontend.gen_files.getDirectory(),
        },
        .config_headers = &.{
            self.metadata.vcs_version,
            llvm.metadata.vcs_revision,
            self.config_h,
            self.clang_artifacts.basic.version_inc,
        },
        .link_libraries = &.{
            llvm.target_artifacts.support,
            llvm.target_artifacts.target_backends.parser,
            llvm.target_artifacts.frontend.open_mp,
        },
        .bundle_compiler_rt = true,
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Lex/CMakeLists.txt
fn buildLex(self: *const Self) Artifact {
    const b = self.b;
    const llvm = self.llvm;

    return self.createClangLibrary(.{
        .name = "clangLex",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, lex.root),
            .files = &lex.sources,
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{self.config_h},
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            llvm.target_artifacts.support,
            llvm.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Rewrite/CMakeLists.txt
fn buildRewrite(self: *const Self) Artifact {
    const b = self.b;
    const llvm = self.llvm;

    return self.createClangLibrary(.{
        .name = "clangRewrite",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, "clang/lib/Rewrite"),
            .files = &.{ "HTMLRewrite.cpp", "Rewriter.cpp", "TokenRewriter.cpp" },
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{self.config_h},
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            self.clang_artifacts.lex,
            llvm.target_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/tree/llvmorg-21.1.8/clang/lib/Tooling
fn buildTooling(self: *const Self) ClangTargetArtifacts.Tooling {
    const b = self.b;
    const llvm = self.llvm;

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Tooling/Core/CMakeLists.txt
    const core = self.createClangLibrary(.{
        .name = "clangToolingCore",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, "clang/lib/Tooling/Core"),
            .files = &.{ "Diagnostic.cpp", "Replacement.cpp" },
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{self.config_h},
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            self.clang_artifacts.lex,
            self.clang_artifacts.rewrite,
            llvm.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Tooling/Inclusions/CMakeLists.txt
    const inclusions = self.createClangLibrary(.{
        .name = "clangToolingInclusions",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, "clang/lib/Tooling/Inclusions"),
            .files = &.{ "HeaderAnalysis.cpp", "HeaderIncludes.cpp", "IncludeStyle.cpp" },
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{self.config_h},
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            self.clang_artifacts.lex,
            core,
            llvm.target_artifacts.support,
        },
    });

    return .{
        .core = core,
        .inclusions = inclusions,
    };
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Format/CMakeLists.txt
fn buildFormat(self: *const Self) Artifact {
    const b = self.b;
    const llvm = self.llvm;

    return self.createClangLibrary(.{
        .name = "clangFormat",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, format.root),
            .files = &format.sources,
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{self.config_h},
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            self.clang_artifacts.lex,
            self.clang_artifacts.tooling.core,
            self.clang_artifacts.tooling.inclusions,
            llvm.target_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/tools/clang-format/CMakeLists.txt
fn buildFormatTool(self: *const Self) Artifact {
    const b = self.b;
    const llvm = self.llvm;

    return self.createClangExecutable(.{
        .name = "clang-format",
        .cxx_source_files = .{
            .root = self.metadata.root.path(b, "clang/tools/clang-format"),
            .files = &.{"ClangFormat.cpp"},
        },
        .additional_include_paths = &.{
            self.metadata.root.path(b, basic.root),
            self.clang_artifacts.basic.gen.getDirectory(),
        },
        .config_headers = &.{ self.config_h, self.clang_artifacts.basic.version_inc },
        .link_libraries = &.{
            self.clang_artifacts.basic.core_lib,
            self.clang_artifacts.format,
            self.clang_artifacts.rewrite,
            self.clang_artifacts.tooling.core,
            llvm.target_artifacts.support,
        },
    });
}

/// Returns all clang artifacts
pub fn allClangArtifacts(self: *const Self) []Artifact {
    var all_artifacts: std.ArrayList(Artifact) = .empty;
    all_artifacts.appendSlice(self.b.allocator, &.{
        self.clang_artifacts.support,
        self.clang_artifacts.basic.core_lib,
        self.clang_artifacts.lex,
        self.clang_artifacts.rewrite,
        self.clang_artifacts.tooling.core,
        self.clang_artifacts.tooling.inclusions,
        self.clang_artifacts.format,
    }) catch @panic("OOM");
    return all_artifacts.items;
}
