//! LLVM Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

const analysis = @import("sources/analysis.zig");
const binary_format = @import("sources/binary_format.zig");
const codegen = @import("sources/codegen.zig");
const dbg_info = @import("sources/dbg_info.zig");
const dependencies = @import("sources/dependencies.zig");
const execution_engine = @import("sources/execution_engine.zig");
const ir = @import("sources/ir.zig");
const kaleidoscope = @import("sources/kaleidoscope.zig");
const lto = @import("sources/lto.zig");
const machine_code = @import("sources/machine_code.zig");
const object = @import("sources/object.zig");
const passes = @import("sources/passes.zig");
const prof_data = @import("sources/prof_data.zig");
const remarks = @import("sources/remarks.zig");
const sandbox_ir = @import("sources/sandbox_ir.zig");
const support = @import("sources/support.zig");
const target_backends = @import("sources/target_backends.zig");
const tblgen = @import("sources/tblgen.zig");
const text_api = @import("sources/text_api.zig");
const transforms = @import("sources/transforms.zig");
const xray = @import("sources/xray.zig");

const Artifact = *std.Build.Step.Compile;

const ThirdParty = struct {
    const siphash_inc = "third-party/siphash/include";
};

const optimize: std.builtin.OptimizeMode = .ReleaseSafe;

const Dependencies = struct {
    const Dependency = struct {
        dependency: *std.Build.Dependency,
        artifact: Artifact,
    };

    zlib: Dependency = undefined,
    libxml2: Dependency = undefined,
    zstd: Dependency = undefined,
};

const Platform = enum {
    host,
    target,

    fn suffix(self: Platform) []const u8 {
        return switch (self) {
            .host => "Host",
            else => "",
        };
    }
};

/// A minimal set of artifacts, compiled for the host arch.
///
/// These are used for the generation of core files for the LLVM pipeline.
/// A distinction between minimal and full must be made to prevent a circular dependency.
const MinimalArtifacts = struct {
    tblgen: Artifact = undefined,
    deps: Dependencies = undefined,
    demangle: Artifact = undefined,
    support: Artifact = undefined,

    gen_vt: *std.Build.Step.WriteFile,
};

/// Artifacts compiled for the actual target
const TargetArtifacts = struct {
    const Bitcode = struct {
        reader: Artifact = undefined,
        writer: Artifact = undefined,
    };

    const MachineCode = struct {
        core_lib: Artifact = undefined,
        parser: Artifact = undefined,
        disassembler: Artifact = undefined,
        analyzer: Artifact = undefined,
    };

    const TextAPI = struct {
        core_lib: Artifact = undefined,
        binary_reader: Artifact = undefined,
    };

    const DebugInfo = struct {
        btf: Artifact = undefined,
        code_view: Artifact = undefined,
        dwarf: struct {
            core_lib: Artifact = undefined,
            low_level: Artifact = undefined,
        } = .{},
        gsym: Artifact = undefined,
        logical_view: Artifact = undefined,
        msf: Artifact = undefined,
        pdb: Artifact = undefined,
        symbolize: Artifact = undefined,
    };

    const ProfileData = struct {
        core_lib: Artifact = undefined,
        coverage: Artifact = undefined,
    };

    const CodeGen = struct {
        core_lib: Artifact = undefined,
        types: Artifact = undefined,
        data: Artifact = undefined,
        selection_dag: Artifact = undefined,
        global_isel: Artifact = undefined,
        asm_printer: Artifact = undefined,
        mir_parser: Artifact = undefined,
    };

    const Transforms = struct {
        utils: Artifact = undefined,
        instrumentation: Artifact = undefined,
        aggressive_inst_combine: Artifact = undefined,
        inst_combine: Artifact = undefined,
        scalar: Artifact = undefined,
        ipo: Artifact = undefined,
        vectorize: Artifact = undefined,
        obj_carc: Artifact = undefined,
        coroutines: Artifact = undefined,
        cf_guard: Artifact = undefined,
        hip_std_par: Artifact = undefined,
    };

    const Frontend = struct {
        gen_files: *std.Build.Step.WriteFile = undefined,

        atomic: Artifact = undefined,
        directive: Artifact = undefined,
        driver: Artifact = undefined,
        hlsl: Artifact = undefined,
        open_acc: Artifact = undefined,
        open_mp: Artifact = undefined,
        offloading: Artifact = undefined,
    };

    const TargetBackends = struct {
        core_lib: Artifact = undefined,
        parser: Artifact = undefined,

        /// Registry of target-specific generated files
        parser_gen: *std.Build.Step.WriteFile = undefined,

        x86: Artifact = undefined,
        aarch64: Artifact = undefined,
        arm: Artifact = undefined,
        riscv: Artifact = undefined,
        wasm: Artifact = undefined,
        xtensa: Artifact = undefined,
        powerpc: Artifact = undefined,
        loong_arch: Artifact = undefined,
    };

    const DwarfLinker = struct {
        core_lib: Artifact = undefined,
        classic: Artifact = undefined,
        parallel: Artifact = undefined,
    };

    const WindowsSupport = struct {
        driver: Artifact = undefined,
        manifest: Artifact = undefined,
    };

    const ToolDrivers = struct {
        lib: Artifact = undefined,
        dlltool: Artifact = undefined,
    };

    const ExecutionEngine = struct {
        core_lib: Artifact = undefined,
        interpreter: Artifact = undefined,
        jit_link: Artifact = undefined,
        mc_jit: Artifact = undefined,
        orc: struct {
            core_lib: Artifact = undefined,
            debugging: Artifact = undefined,
            shared: Artifact = undefined,
            target_process: Artifact = undefined,
        } = .{},
        runtime_dyld: Artifact = undefined,
    };

    deps: Dependencies = .{},
    demangle: Artifact = undefined,
    support: Artifact = undefined,
    bitstream_reader: Artifact = undefined,
    binary_format: Artifact = undefined,
    remarks: Artifact = undefined,
    core: Artifact = undefined,

    bitcode: Bitcode = .{},
    machine_code: MachineCode = .{},
    asm_parser: Artifact = undefined,
    ir_reader: Artifact = undefined,
    text_api: TextAPI = .{},
    object: Artifact = undefined,
    object_yaml: Artifact = undefined,
    debug_info: DebugInfo = .{},
    profile_data: ProfileData = .{},
    analysis: Artifact = undefined,

    codegen: CodeGen = .{},
    sandbox_ir: Artifact = undefined,
    frontend: Frontend = .{},
    transforms: Transforms = .{},
    linker: Artifact = undefined,

    target_backends: TargetBackends = .{},
    passes: Artifact = undefined,
    ir_printer: Artifact = undefined,

    option: Artifact = undefined,
    obj_copy: Artifact = undefined,
    dwarf_linker: DwarfLinker = .{},
    dwp: Artifact = undefined,
    file_check: Artifact = undefined,
    extensions: Artifact = undefined,
    lto: Artifact = undefined,
    xray: Artifact = undefined,
    windows_support: WindowsSupport = .{},
    tool_drivers: ToolDrivers = .{},
    execution_engine: ExecutionEngine = .{},

    /// Registry of generated intrinsics (e.g. Attributes.inc)
    intrinsics_gen: *std.Build.Step.WriteFile = undefined,
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

pub const common_llvm_cxx_flags = [_][]const u8{
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

const Self = @This();

b: *std.Build,
llvm: struct {
    upstream: *std.Build.Dependency,
    root: std.Build.LazyPath,
    llvm_include: std.Build.LazyPath,
    vcs_revision: *std.Build.Step.ConfigHeader,
    extension_def: *std.Build.Step.WriteFile,
},

minimal_artifacts: MinimalArtifacts,

/// Used to convert the actual target 'td's into 'inc's
full_tblgen: Artifact = undefined,

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
            .extension_def = b.addWriteFile("llvm/Support/Extension.def", "#undef HANDLE_EXTENSION"),
        },
        .minimal_artifacts = .{
            .gen_vt = b.addWriteFiles(),
        },
        .target = target,
    };
}

pub fn build(self: *Self, config: struct {
    behavior: union(enum) {
        supplemental_cxx_flags: []const []const u8,
        package,
    },
    skip_link_test: bool = true,
    auto_install: bool = false,
}) !void {
    const b = self.b;

    // Host Dependencies for TableGen
    self.minimal_artifacts.deps = try self.compileDeps(.host);
    self.minimal_artifacts.demangle = self.compileDemangle(.host);
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
    self.target_artifacts.deps = try self.compileDeps(.target);
    self.target_artifacts.demangle = self.compileDemangle(.target);
    self.target_artifacts.support = try self.compileSupport(.{
        .platform = .target,
        .deps = self.target_artifacts.deps,
        .minimal = false,
    });

    self.target_artifacts.target_backends.parser_gen = self.runTargetParserGen();
    self.target_artifacts.target_backends.parser = self.compileTargetParser();
    self.target_artifacts.bitstream_reader = self.compileBitstreamReader();
    self.target_artifacts.binary_format = self.compileBinaryFormat();
    self.target_artifacts.remarks = self.compileRemarks();
    self.target_artifacts.intrinsics_gen = self.runIntrinsicsGen();
    self.target_artifacts.core = self.compileCore();

    self.target_artifacts.bitcode.reader = self.compileBitcodeReader();
    self.target_artifacts.machine_code = self.compileMC();
    self.target_artifacts.asm_parser = self.compileAsmParser();
    self.target_artifacts.ir_reader = self.compileIRReader();
    self.target_artifacts.text_api.core_lib = self.compileTextAPI();
    self.target_artifacts.object = self.compileObject();

    self.target_artifacts.debug_info = self.compileDebugInfo();
    self.target_artifacts.object_yaml = self.compileObjectYAML();
    self.target_artifacts.profile_data = self.compileProfileData();
    self.target_artifacts.analysis = self.compileAnalysis();
    self.target_artifacts.bitcode.writer = self.compileBitcodeWriter();
    self.target_artifacts.text_api.binary_reader = self.compileTextAPIBinaryReader();

    self.target_artifacts.codegen.types = self.compileCodeGenTypes();
    self.target_artifacts.codegen.data = self.compileCGData();
    self.target_artifacts.sandbox_ir = self.compileSandboxIR();
    self.target_artifacts.transforms.utils = self.compileTransformUtils();
    self.target_artifacts.transforms.instrumentation = self.compileInstrumentation();
    self.target_artifacts.frontend = self.compileFrontend();
    self.target_artifacts.linker = self.compileLinker();

    self.target_artifacts.transforms.inst_combine = self.compileInstCombine();
    self.target_artifacts.transforms.aggressive_inst_combine = self.compileAggressiveInstCombine();
    self.target_artifacts.transforms.cf_guard = self.compileCFGuard();
    self.target_artifacts.transforms.hip_std_par = self.compileHipStdPar();
    self.target_artifacts.transforms.obj_carc = self.compileObjCARC();
    self.target_artifacts.transforms.scalar = self.compileScalar();
    self.target_artifacts.transforms.vectorize = self.compileVectorize();
    self.target_artifacts.transforms.ipo = self.compileIPO();
    self.target_artifacts.transforms.coroutines = self.compileCoroutines();

    self.target_artifacts.target_backends.core_lib = self.compileTarget();
    self.target_artifacts.codegen.core_lib = self.compileCodeGen();
    self.target_artifacts.codegen.selection_dag = self.compileSelectionDAG();
    self.target_artifacts.codegen.global_isel = self.compileGlobalISel();
    self.target_artifacts.codegen.asm_printer = self.compileAsmPrinter();
    self.target_artifacts.codegen.mir_parser = self.compileMIRParser();
    self.target_artifacts.ir_printer = self.compileIRPrinter();
    self.target_artifacts.passes = self.compilePasses();

    self.target_artifacts.option = self.compileOption();
    self.target_artifacts.obj_copy = self.compileObjCopy();
    self.target_artifacts.dwarf_linker = self.compileDwarfLinker();
    self.target_artifacts.dwp = self.compileDWP();
    self.target_artifacts.file_check = self.compileFileCheck();
    self.target_artifacts.extensions = self.compileExtensions();
    self.target_artifacts.lto = self.compileLTO();
    self.target_artifacts.xray = self.compileXRay();
    self.target_artifacts.windows_support = self.compileWindowsSupport();
    self.target_artifacts.tool_drivers = self.compileToolDrivers();

    self.target_artifacts.target_backends.x86 = self.compileX86();
    self.target_artifacts.target_backends.aarch64 = self.compileAArch64();
    self.target_artifacts.target_backends.arm = self.compileArm();
    self.target_artifacts.target_backends.riscv = self.compileRISCV();
    self.target_artifacts.target_backends.wasm = self.compileWebAssembly();
    self.target_artifacts.target_backends.xtensa = self.compileXtensa();
    self.target_artifacts.target_backends.powerpc = self.compilePowerPC();
    self.target_artifacts.target_backends.loong_arch = self.compileLoongArch();

    self.target_artifacts.execution_engine = self.compileExecutionEngine();

    // Packaging ignores test builds
    switch (config.behavior) {
        .supplemental_cxx_flags => |cxx_flags| {
            self.createLinkTest(cxx_flags);

            const kaleidoscope_install_options: std.Build.Step.InstallArtifact.Options = .{
                .dest_dir = .{
                    .override = .{
                        .custom = "kaleidoscope",
                    },
                },
            };

            if (config.auto_install and b.option(
                bool,
                "all-chapters",
                "build all kaleidoscope chapters (dangerous)",
            ) orelse false) {
                for (std.enums.values(kaleidoscope.KaleidoscopeChapter)) |chapter| {
                    const exe = chapter.compile(self);
                    const install_artifact = b.addInstallArtifact(exe, kaleidoscope_install_options);
                    b.getInstallStep().dependOn(&install_artifact.step);
                }
            } else if (b.option(
                kaleidoscope.KaleidoscopeChapter,
                "Chapter",
                "the kaleidoscope chapter to run",
            )) |chapter| {
                const exe = chapter.compile(self);
                const run_cmd = b.addRunArtifact(exe);
                run_cmd.step.dependOn(b.getInstallStep());

                if (b.args) |args| {
                    run_cmd.addArgs(args);
                }

                const kaleidoscope_step = b.step("kaleidoscope", "Run a Kaleidoscope Chapter's executable");
                kaleidoscope_step.dependOn(&run_cmd.step);

                if (config.auto_install) {
                    const install_artifact = b.addInstallArtifact(exe, kaleidoscope_install_options);
                    b.getInstallStep().dependOn(&install_artifact.step);
                }
            }
        },
        .package => {},
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

const AddCSourceFileOptions = struct {
    root: std.Build.LazyPath,
    files: []const []const u8,
    flags: []const []const u8 = &common_llvm_cxx_flags,
    language: ?std.Build.Module.CSourceLanguage = null,
};

/// Creates a module and corresponding library, defaulting to the target platform.
/// 'llvm/include' is implicitly included here
fn createLLVMLibrary(self: *const Self, config: struct {
    name: []const u8,
    cxx_source_files: ?AddCSourceFileOptions,
    additional_include_paths: ?[]const std.Build.LazyPath = null,
    config_headers: ?[]const *std.Build.Step.ConfigHeader = null,
    link_libraries: ?[]const Artifact = null,
    target_override: ?std.Build.ResolvedTarget = null,
    bundle_compiler_rt: ?bool = null,
}) Artifact {
    const mod = self.createTargetModule();
    if (config.target_override) |target| {
        mod.resolved_target = target;
    }

    if (config.cxx_source_files) |cxx_source_files| mod.addCSourceFiles(.{
        .root = cxx_source_files.root,
        .files = cxx_source_files.files,
        .flags = cxx_source_files.flags,
        .language = cxx_source_files.language,
    });

    if (config.additional_include_paths) |addtl_includes| for (addtl_includes) |include| {
        mod.addIncludePath(include);
    };
    mod.addIncludePath(self.llvm.llvm_include);

    if (config.config_headers) |config_headers| for (config_headers) |config_header| {
        mod.addConfigHeader(config_header);
    };

    if (config.link_libraries) |libs| for (libs) |lib| {
        mod.linkLibrary(lib);
    };

    const lib = self.b.addLibrary(.{
        .name = config.name,
        .root_module = mod,
    });
    lib.bundle_compiler_rt = config.bundle_compiler_rt;
    return lib;
}

/// Creates a custom runnable target to test LLVM linkage against a C++23 source file
fn createLinkTest(self: *const Self, cxx_flags: []const []const u8) void {
    const b = self.b;
    const mod = self.createHostModule();
    mod.optimize = .Debug;

    mod.addCSourceFile(.{
        .file = b.path("packages/llvm/link_test.cpp"),
        .flags = cxx_flags,
    });
    mod.addSystemIncludePath(self.llvm.llvm_include);

    for (self.allTargetArtifacts()) |lib| {
        mod.linkLibrary(lib);
    }

    const exe = b.addExecutable(.{
        .name = "llvm_link_test",
        .root_module = mod,
    });

    const run_cmd = b.addRunArtifact(exe);
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("link-test", "Run the llvm link test executable");
    run_step.dependOn(&run_cmd.step);
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
            self.config_h,          self.llvm_config_h,
            self.abi_breaking_h,    self.targets_h,
            self.asm_parsers_def,   self.asm_printers_def,
            self.disassemblers_def, self.target_exegesis_def,
            self.target_mcas_def,   self.targets_def,
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
}) !Artifact {
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
        .files = &support.reg_sources,
        .flags = &.{"-std=c11"},
    });

    mod.addCSourceFiles(.{
        .root = self.llvm.root.path(b, support.blake3_root),
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
fn compileDemangle(self: *const Self, platform: Platform) Artifact {
    return self.createLLVMLibrary(.{
        .name = self.b.fmt("LLVMDemangle{s}", .{platform.suffix()}),
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Demangle"),
            .files = &.{
                "DLangDemangle.cpp",          "Demangle.cpp",
                "ItaniumDemangle.cpp",        "MicrosoftDemangle.cpp",
                "MicrosoftDemangleNodes.cpp", "RustDemangle.cpp",
            },
        },
        .target_override = switch (platform) {
            .host => self.b.graph.host,
            .target => null,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/BinaryFormat/CMakeLists.txt
fn compileBinaryFormat(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMBinaryFormat",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, binary_format.root),
            .files = &binary_format.sources,
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Remarks/CMakeLists.txt
fn compileRemarks(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMRemarks",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, remarks.root),
            .files = &remarks.sources,
        },
        .additional_include_paths = &.{self.minimal_artifacts.gen_vt.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.bitstream_reader,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Bitstream/Reader/CMakeLists.txt
fn compileBitstreamReader(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMBitstreamReader",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Bitstream/Reader"),
            .files = &.{"BitstreamReader.cpp"},
        },
        .link_libraries = &.{self.target_artifacts.support},
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
fn compileTargetParser(self: *Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMTargetParser",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, target_backends.parser_root),
            .files = &target_backends.parser_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.target_backends.parser_gen.getDirectory()},
        .link_libraries = &.{self.target_artifacts.support},
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
fn compileCore(self: *Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCore",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, ir.root),
            .files = &ir.sources,
        },
        .additional_include_paths = &.{
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.demangle,
            self.target_artifacts.remarks,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Bitcode/Reader/CMakeLists.txt
fn compileBitcodeReader(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMBitReader",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Bitcode/Reader"),
            .files = &.{
                "BitcodeAnalyzer.cpp", "BitReader.cpp",
                "BitcodeReader.cpp",   "MetadataLoader.cpp",
                "ValueList.cpp",
            },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.bitstream_reader,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/CMakeLists.txt
fn compileMC(self: *const Self) TargetArtifacts.MachineCode {
    const b = self.b;
    var mc: TargetArtifacts.MachineCode = .{};

    mc.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMMC",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, machine_code.mc_root),
            .files = &machine_code.mc_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.binary_format,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/MCDisassembler/CMakeLists.txt
    mc.disassembler = self.createLLVMLibrary(.{
        .name = "LLVMMCDisassembler",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, machine_code.disassembler_root),
            .files = &machine_code.disassembler_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            mc.core_lib,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MC/MCParser/CMakeLists.txt
    mc.parser = self.createLLVMLibrary(.{
        .name = "LLVMMCParser",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, machine_code.parser_root),
            .files = &machine_code.parser_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            mc.core_lib,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/MCA/CMakeLists.txt
    mc.analyzer = self.createLLVMLibrary(.{
        .name = "LLVMMCA",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, machine_code.mca_root),
            .files = &machine_code.mca_sources,
        },
        .link_libraries = &.{
            mc.core_lib,
            self.target_artifacts.support,
        },
    });

    return mc;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/AsmParser/CMakeLists.txt
fn compileAsmParser(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMAsmParser",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/AsmParser"),
            .files = &.{ "LLLexer.cpp", "LLParser.cpp", "Parser.cpp" },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/IRReader/CMakeLists.txt
fn compileIRReader(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMIRReader",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/IRReader"),
            .files = &.{"IRReader.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.asm_parser,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.core,
            self.target_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TextAPI/CMakeLists.txt
fn compileTextAPI(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMTextAPI",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, text_api.root),
            .files = &text_api.sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.binary_format,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Object/CMakeLists.txt
fn compileObject(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMObject",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, object.root),
            .files = &object.sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .config_headers = &.{self.llvm.vcs_revision},
        .link_libraries = &.{
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.ir_reader,
            self.target_artifacts.binary_format,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.text_api.core_lib,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/CMakeLists.txt
fn compileDebugInfo(self: *const Self) TargetArtifacts.DebugInfo {
    const b = self.b;
    var debug_info: TargetArtifacts.DebugInfo = .{};

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/BTF/CMakeLists.txt
    debug_info.btf = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoBTF",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.btf_root),
            .files = &dbg_info.btf_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/CodeView/CMakeLists.txt
    debug_info.code_view = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoCodeView",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.code_view_root),
            .files = &dbg_info.code_view_sources,
        },
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/DWARF/LowLevel/CMakeLists.txt
    debug_info.dwarf.low_level = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoDWARFLowLevel",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.dwarf_ll_root),
            .files = &dbg_info.dwarf_ll_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/DWARF/CMakeLists.txt
    debug_info.dwarf.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoDWARF",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.dwarf_root),
            .files = &dbg_info.dwarf_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            debug_info.dwarf.low_level,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/GSYM/CMakeLists.txt
    debug_info.gsym = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoGSYM",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.gsym_root),
            .files = &dbg_info.gsym_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            debug_info.dwarf.core_lib,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/MSF/CMakeLists.txt
    debug_info.msf = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoMSF",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.msf_root),
            .files = &dbg_info.msf_sources,
        },
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/PDB/CMakeLists.txt
    debug_info.pdb = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoPDB",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.pdb_root),
            .files = &dbg_info.pdb_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.support,
            debug_info.code_view,
            debug_info.msf,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/LogicalView/CMakeLists.txt
    debug_info.logical_view = self.createLLVMLibrary(.{
        .name = "LLVMDebugInfoLogicalView",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.logical_view_root),
            .files = &dbg_info.logical_view_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.demangle,
            self.target_artifacts.object,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            debug_info.code_view,
            debug_info.dwarf.core_lib,
            debug_info.dwarf.low_level,
            debug_info.pdb,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DebugInfo/Symbolize/CMakeLists.txt
    debug_info.symbolize = self.createLLVMLibrary(.{
        .name = "LLVMSymbolize",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.symbolize_root),
            .files = &dbg_info.symbolize_sources,
        },
        .link_libraries = &.{
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.demangle,
            self.target_artifacts.target_backends.parser,
            debug_info.dwarf.core_lib,
            debug_info.gsym,
            debug_info.pdb,
            debug_info.btf,
        },
    });

    return debug_info;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ObjectYAML/CMakeLists.txt
fn compileObjectYAML(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMObjectYAML",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, object.yaml_root),
            .files = &object.yaml_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.debug_info.code_view,
            self.target_artifacts.machine_code.core_lib,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ProfileData/CMakeLists.txt
fn compileProfileData(self: *const Self) TargetArtifacts.ProfileData {
    const b = self.b;
    var profile_data: TargetArtifacts.ProfileData = .{};

    profile_data.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMProfileData",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, prof_data.prof_data_root),
            .files = &prof_data.prof_data_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.bitstream_reader,
            self.target_artifacts.core,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.demangle,
            self.target_artifacts.debug_info.symbolize,
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.debug_info.dwarf.low_level,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ProfileData/Coverage/CMakeLists.txt
    profile_data.coverage = self.createLLVMLibrary(.{
        .name = "LLVMCoverage",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, prof_data.coverage_root),
            .files = &prof_data.coverage_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.object,
            profile_data.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    return profile_data;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Analysis/CMakeLists.txt
fn compileAnalysis(self: *const Self) Artifact {
    var library = self.createLLVMLibrary(.{
        .name = "LLVMAnalysis",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, analysis.root),
            .files = &analysis.sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.core,
            self.target_artifacts.object,
            self.target_artifacts.profile_data.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // Constant folding needs highly specific math
    library.root_module.addCSourceFile(.{
        .file = self.llvm.root.path(self.b, analysis.constant_folding),
        .flags = &common_llvm_cxx_flags ++ .{analysis.extra_constant_folding_flag},
    });
    return library;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Bitcode/Writer/CMakeLists.txt
fn compileBitcodeWriter(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMBitWriter",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Bitcode/Writer"),
            .files = &.{
                "BitWriter.cpp",         "BitcodeWriter.cpp",
                "BitcodeWriterPass.cpp", "ValueEnumerator.cpp",
            },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.profile_data.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/TextAPI/BinaryReader/CMakeLists.txt
fn compileTextAPIBinaryReader(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMTextAPIBinaryReader",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/TextAPI/BinaryReader"),
            .files = &.{"DylibReader.cpp"},
        },
        .link_libraries = &.{
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.object,
            self.target_artifacts.text_api.core_lib,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGenTypes/CMakeLists.txt
fn compileCodeGenTypes(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCodeGenTypes",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/CodeGenTypes"),
            .files = &.{"LowLevelType.cpp"},
        },
        .additional_include_paths = &.{self.minimal_artifacts.gen_vt.getDirectory()},
        .link_libraries = &.{self.target_artifacts.support},
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CGData/CMakeLists.txt
fn compileCGData(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCGData",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, codegen.data_root),
            .files = &codegen.data_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.bitcode.writer,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.object,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/SandboxIR/CMakeLists.txt
fn compileSandboxIR(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMSandboxIR",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, sandbox_ir.root),
            .files = &sandbox_ir.sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.analysis,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/Instrumentation/CMakeLists.txt
fn compileInstrumentation(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMInstrumentation",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.instrumentation_root),
            .files = &transforms.instrumentation_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.demangle,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.profile_data.core_lib,
        },
    });
}

fn compileFrontend(self: *Self) TargetArtifacts.Frontend {
    const b = self.b;
    var frontend: TargetArtifacts.Frontend = .{
        .gen_files = b.addWriteFiles(),
    };

    // OpenMP gen files
    _ = self.synthesizeHeader(frontend.gen_files, .{
        .tblgen = self.full_tblgen,
        .name = "OMP",
        .td_file = "llvm/include/llvm/Frontend/OpenMP/OMP.td",
        .instruction = .{ .action = "-gen-directive-impl" },
        .virtual_path = "llvm/Frontend/OpenMP/OMP.inc",
    });

    _ = self.synthesizeHeader(frontend.gen_files, .{
        .tblgen = self.full_tblgen,
        .name = "OMP.h",
        .td_file = "llvm/include/llvm/Frontend/OpenMP/OMP.td",
        .instruction = .{ .action = "-gen-directive-decl" },
        .virtual_path = "llvm/Frontend/OpenMP/OMP.h.inc",
    });

    // OpenACC gen files
    _ = self.synthesizeHeader(frontend.gen_files, .{
        .tblgen = self.full_tblgen,
        .name = "ACC",
        .td_file = "llvm/include/llvm/Frontend/OpenACC/ACC.td",
        .instruction = .{ .action = "-gen-directive-impl" },
        .virtual_path = "llvm/Frontend/OpenACC/ACC.inc",
    });

    _ = self.synthesizeHeader(frontend.gen_files, .{
        .tblgen = self.full_tblgen,
        .name = "ACC.h",
        .td_file = "llvm/include/llvm/Frontend/OpenACC/ACC.td",
        .instruction = .{ .action = "-gen-directive-decl" },
        .virtual_path = "llvm/Frontend/OpenACC/ACC.h.inc",
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/Atomic/CMakeLists.txt
    frontend.atomic = self.createLLVMLibrary(.{
        .name = "LLVMFrontendAtomic",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/Atomic"),
            .files = &.{"Atomic.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.analysis,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/Directive/CMakeLists.txt
    frontend.directive = self.createLLVMLibrary(.{
        .name = "LLVMFrontendDirective",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/Directive"),
            .files = &.{"Spelling.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/Driver/CMakeLists.txt
    frontend.driver = self.createLLVMLibrary(.{
        .name = "LLVMFrontendDriver",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/Driver"),
            .files = &.{"CodeGenOptions.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.analysis,
            self.target_artifacts.transforms.instrumentation,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/HLSL/CMakeLists.txt
    frontend.hlsl = self.createLLVMLibrary(.{
        .name = "LLVMFrontendHLSL",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/HLSL"),
            .files = &.{
                "CBuffer.cpp",                  "HLSLResource.cpp",
                "HLSLRootSignature.cpp",        "RootSignatureMetadata.cpp",
                "RootSignatureValidations.cpp",
            },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.core,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/Offloading/CMakeLists.txt
    frontend.offloading = self.createLLVMLibrary(.{
        .name = "LLVMFrontendOffloading",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/Offloading"),
            .files = &.{ "Utility.cpp", "OffloadWrapper.cpp" },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.object_yaml,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/OpenMP/CMakeLists.txt
    frontend.open_mp = self.createLLVMLibrary(.{
        .name = "LLVMFrontendOpenMP",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/OpenMP"),
            .files = &.{
                "OMP.cpp",          "OMPContext.cpp",
                "OMPIRBuilder.cpp", "DirectiveNameParser.cpp",
            },
        },
        .additional_include_paths = &.{
            frontend.gen_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.object,
            self.target_artifacts.text_api.core_lib,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Frontend/OpenACC/CMakeLists.txt
    frontend.open_acc = self.createLLVMLibrary(.{
        .name = "LLVMFrontendOpenACC",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/Frontend/OpenACC"),
            .files = &.{"ACC.cpp"},
        },
        .additional_include_paths = &.{
            frontend.gen_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{ self.target_artifacts.support, frontend.directive },
    });

    return frontend;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Linker/CMakeLists.txt
fn compileLinker(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMLinker",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Linker"),
            .files = &.{ "IRMover.cpp", "LinkModules.cpp" },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/Utils/CMakeLists.txt
fn compileTransformUtils(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMTransformUtils",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.utils_root),
            .files = &transforms.utils_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/InstCombine/CMakeLists.txt
fn compileInstCombine(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMInstCombine",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.inst_combine_root),
            .files = &transforms.inst_combine_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/AggressiveInstCombine/CMakeLists.txt
fn compileAggressiveInstCombine(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMAggressiveInstCombine",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.aggressive_inst_root),
            .files = &transforms.aggressive_inst_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/CFGuard/CMakeLists.txt
fn compileCFGuard(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCFGuard",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Transforms/CFGuard"),
            .files = &.{"CFGuard.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/HipStdPar/CMakeLists.txt
fn compileHipStdPar(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMHipStdPar",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Transforms/HipStdPar"),
            .files = &.{"HipStdPar.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/ObjCARC/CMakeLists.txt
fn compileObjCARC(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMObjCARCOpts",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.obj_carc_root),
            .files = &transforms.obj_carc_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/Scalar/CMakeLists.txt
fn compileScalar(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMScalarOpts",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.scalar_root),
            .files = &transforms.scalar_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.transforms.aggressive_inst_combine,
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.transforms.inst_combine,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/Vectorize/CMakeLists.txt
fn compileVectorize(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMVectorize",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.vectorize_root),
            .files = &transforms.vectorize_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.sandbox_ir,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/IPO/CMakeLists.txt
fn compileIPO(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMipo",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.ipo_root),
            .files = &transforms.ipo_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.frontend.gen_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.transforms.aggressive_inst_combine,
            self.target_artifacts.analysis,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.bitcode.writer,
            self.target_artifacts.core,
            self.target_artifacts.frontend.open_mp,
            self.target_artifacts.transforms.inst_combine,
            self.target_artifacts.ir_reader,
            self.target_artifacts.demangle,
            self.target_artifacts.linker,
            self.target_artifacts.object,
            self.target_artifacts.profile_data.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.transforms.vectorize,
            self.target_artifacts.transforms.instrumentation,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Transforms/Coroutines/CMakeLists.txt
fn compileCoroutines(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCoroutines",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, transforms.coroutines_root),
            .files = &transforms.coroutines_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.transforms.ipo,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.support,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/CMakeLists.txt
fn compileTarget(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMTarget",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, target_backends.root),
            .files = &target_backends.base_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGen/CMakeLists.txt
fn compileCodeGen(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMCodeGen",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, codegen.codegen_root),
            .files = &codegen.codegen_sources,
        },
        .additional_include_paths = &.{
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.bitcode.writer,
            self.target_artifacts.codegen.data,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.obj_carc,
            self.target_artifacts.profile_data.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGen/SelectionDAG/CMakeLists.txt
fn compileSelectionDAG(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMSelectionDAG",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, codegen.selection_dag_root),
            .files = &codegen.selection_dag_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGen/GlobalISel/CMakeLists.txt
fn compileGlobalISel(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMGlobalISel",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, codegen.global_isel_root),
            .files = &codegen.global_isel_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGen/AsmPrinter/CMakeLists.txt
fn compileAsmPrinter(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMAsmPrinter",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, codegen.asm_printer_root),
            .files = &codegen.asm_printer_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .config_headers = &.{self.llvm.vcs_revision},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.debug_info.code_view,
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.debug_info.dwarf.low_level,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.remarks,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/CodeGen/MIRParser/CMakeLists.txt
fn compileMIRParser(self: *const Self) Artifact {
    const root = self.llvm.root.path(self.b, codegen.mir_parser_root);
    return self.createLLVMLibrary(.{
        .name = "LLVMMIRParser",
        .cxx_source_files = .{
            .root = root,
            .files = &codegen.mir_parser_sources,
        },
        .additional_include_paths = &.{
            root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.asm_parser,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/IRPrinter/CMakeLists.txt
fn compileIRPrinter(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMIRPrinter",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/IRPrinter"),
            .files = &.{"IRPrintingPasses.cpp"},
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.analysis,
            self.target_artifacts.core,
            self.target_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Passes/CMakeLists.txt
fn compilePasses(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMPasses",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, passes.root),
            .files = &passes.sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.transforms.aggressive_inst_combine,
            self.target_artifacts.analysis,
            self.target_artifacts.transforms.cf_guard,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.core,
            self.target_artifacts.transforms.coroutines,
            self.target_artifacts.transforms.hip_std_par,
            self.target_artifacts.transforms.ipo,
            self.target_artifacts.transforms.inst_combine,
            self.target_artifacts.ir_printer,
            self.target_artifacts.transforms.obj_carc,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.transforms.vectorize,
            self.target_artifacts.transforms.instrumentation,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Option/CMakeLists.txt
fn compileOption(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMOption",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Option"),
            .files = &.{
                "Arg.cpp",    "ArgList.cpp",
                "Option.cpp", "OptTable.cpp",
            },
        },
        .link_libraries = &.{self.target_artifacts.support},
    });
}

fn compileObjCopy(self: *const Self) Artifact {
    const copy_root = self.llvm.root.path(self.b, object.copy_root);
    return self.createLLVMLibrary(.{
        .name = "LLVMObjCopy",
        .cxx_source_files = .{
            .root = copy_root,
            .files = &object.copy_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            copy_root,
            self.llvm.root.path(self.b, object.copy_coff_root),
            self.llvm.root.path(self.b, object.copy_coff_root),
            self.llvm.root.path(self.b, object.copy_elf_root),
            self.llvm.root.path(self.b, object.copy_macho_root),
            self.llvm.root.path(self.b, object.copy_wasm_root),
            self.llvm.root.path(self.b, object.copy_xcoff_root),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.machine_code.core_lib,
        },
    });
}

/// https://github.com/llvm/llvm-project/tree/llvmorg-21.1.8/llvm/lib/DWARFLinker/CMakeLists.txt
fn compileDwarfLinker(self: *const Self) TargetArtifacts.DwarfLinker {
    const b = self.b;
    var linker: TargetArtifacts.DwarfLinker = .{};

    linker.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMDWARFLinker",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.dwarf_linker_root),
            .files = &dbg_info.dwarf_linker_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DWARFLinker/Parallel/CMakeLists.txt
    const parallel_root = self.llvm.root.path(b, dbg_info.dwarf_linker_parallel_root);
    linker.parallel = self.createLLVMLibrary(.{
        .name = "LLVMDWARFLinkerParallel",
        .cxx_source_files = .{
            .root = parallel_root,
            .files = &dbg_info.dwarf_linker_parallel_sources,
        },
        .additional_include_paths = &.{
            parallel_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.debug_info.dwarf.low_level,
            linker.core_lib,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DWARFLinker/Classic/CMakeLists.txt
    linker.classic = self.createLLVMLibrary(.{
        .name = "LLVMDWARFLinkerClassic",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, dbg_info.dwarf_linker_classic_root),
            .files = &dbg_info.dwarf_linker_classic_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.debug_info.dwarf.low_level,
            linker.core_lib,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    return linker;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/DWP/CMakeLists.txt
fn compileDWP(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMDWP",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/DWP"),
            .files = &.{ "DWP.cpp", "DWPError.cpp" },
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.debug_info.dwarf.core_lib,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/FileCheck/CMakeLists.txt
fn compileFileCheck(self: *const Self) Artifact {
    const root = self.llvm.root.path(self.b, "llvm/lib/FileCheck");
    return self.createLLVMLibrary(.{
        .name = "LLVMFileCheck",
        .cxx_source_files = .{
            .root = root,
            .files = &.{"FileCheck.cpp"},
        },
        .additional_include_paths = &.{root},
        .link_libraries = &.{self.target_artifacts.support},
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Extensions/CMakeLists.txt
fn compileExtensions(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMExtensions",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, "llvm/lib/Extensions"),
            .files = &.{"Extensions.cpp"},
        },
        .additional_include_paths = &.{self.llvm.extension_def.getDirectory()},
        .link_libraries = &.{self.target_artifacts.support},
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/LTO/CMakeLists.txt
fn compileLTO(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMLTO",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, lto.root),
            .files = &lto.sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.llvm.extension_def.getDirectory(),
        },
        .config_headers = &.{self.llvm.vcs_revision},
        .link_libraries = &.{
            self.target_artifacts.transforms.aggressive_inst_combine,
            self.target_artifacts.analysis,
            self.target_artifacts.binary_format,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.bitcode.writer,
            self.target_artifacts.codegen.data,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.extensions,
            self.target_artifacts.transforms.ipo,
            self.target_artifacts.transforms.inst_combine,
            self.target_artifacts.transforms.instrumentation,
            self.target_artifacts.linker,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.obj_carc,
            self.target_artifacts.object,
            self.target_artifacts.passes,
            self.target_artifacts.remarks,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/XRay/CMakeLists.txt
fn compileXRay(self: *const Self) Artifact {
    return self.createLLVMLibrary(.{
        .name = "LLVMXRay",
        .cxx_source_files = .{
            .root = self.llvm.root.path(self.b, xray.root),
            .files = &xray.sources,
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.object,
            self.target_artifacts.target_backends.parser,
        },
    });
}

fn compileWindowsSupport(self: *const Self) TargetArtifacts.WindowsSupport {
    const b = self.b;
    var windows_support: TargetArtifacts.WindowsSupport = .{};

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/WindowsDriver/CMakeLists.txt
    windows_support.driver = self.createLLVMLibrary(.{
        .name = "LLVMWindowsDriver",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/WindowsDriver"),
            .files = &.{"MSVCPaths.cpp"},
        },
        .link_libraries = &.{
            self.target_artifacts.option,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/WindowsManifest/CMakeLists.txt
    windows_support.manifest = self.createLLVMLibrary(.{
        .name = "LLVMWindowsManifest",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/WindowsManifest"),
            .files = &.{"WindowsManifestMerger.cpp"},
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.deps.libxml2.artifact,
        },
    });

    return windows_support;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ToolDrivers/CMakeLists.txt
fn compileToolDrivers(self: *const Self) TargetArtifacts.ToolDrivers {
    const b = self.b;
    var tool_drivers: TargetArtifacts.ToolDrivers = .{};

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ToolDrivers/llvm-dlltool/CMakeLists.txt
    const dll_options = self.generateTblgenInc(.{
        .tblgen = self.full_tblgen,
        .name = "Options",
        .td_file = "llvm/lib/ToolDrivers/llvm-dlltool/Options.td",
        .instruction = .{ .action = "-gen-opt-parser-defs" },
    });

    tool_drivers.dlltool = self.createLLVMLibrary(.{
        .name = "LLVMDlltoolDriver",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/ToolDrivers/llvm-dlltool"),
            .files = &.{"DlltoolDriver.cpp"},
        },
        .additional_include_paths = &.{dll_options.dirname()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.option,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ToolDrivers/llvm-lib/CMakeLists.txt
    const lib_options = self.generateTblgenInc(.{
        .tblgen = self.full_tblgen,
        .name = "Options",
        .td_file = "llvm/lib/ToolDrivers/llvm-lib/Options.td",
        .instruction = .{ .action = "-gen-opt-parser-defs" },
    });

    tool_drivers.lib = self.createLLVMLibrary(.{
        .name = "LLVMLibDriver",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, "llvm/lib/ToolDrivers/llvm-lib"),
            .files = &.{"LibDriver.cpp"},
        },
        .additional_include_paths = &.{
            lib_options.dirname(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.object,
            self.target_artifacts.option,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    return tool_drivers;
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
fn compileAArch64(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.AArch64;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/Utils/CMakeLists.txt
    const utils_root = self.llvm.root.path(b, Backend.utils_root);
    const utils = self.createLLVMLibrary(.{
        .name = "LLVMAArch64Utils",
        .cxx_source_files = .{
            .root = utils_root,
            .files = &Backend.utils_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            utils_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.support,
            self.target_artifacts.core,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMAArch64Info",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMAArch64Desc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            info,
            utils,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMAArch64Disassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMAArch64AsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMAArch64CodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.transforms.cf_guard,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.transforms.vectorize,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/CMakeLists.txt
fn compileX86(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.X86;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMX86Info",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMX86Desc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMX86Disassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMX86AsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/X86/Disassembler/CMakeLists.txt
    const mca_root = self.llvm.root.path(b, Backend.mca_root);
    const mca = self.createLLVMLibrary(.{
        .name = "LLVMX86TargetMCA",
        .cxx_source_files = .{
            .root = mca_root,
            .files = &Backend.mca_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            mca_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.analyzer,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMX86CodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            disassembler,
            mca,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.transforms.cf_guard,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.transforms.vectorize,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/CMakeLists.txt
fn compileArm(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.Arm;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/Utils/CMakeLists.txt
    const utils_root = self.llvm.root.path(b, Backend.utils_root);
    const utils = self.createLLVMLibrary(.{
        .name = "LLVMARMUtils",
        .cxx_source_files = .{
            .root = utils_root,
            .files = &Backend.utils_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            utils_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMARMInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMARMDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            info,
            utils,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMARMDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMARMAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/ARM/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMARMCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.transforms.cf_guard,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/CMakeLists.txt
fn compileRISCV(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.RiscV;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    // RISCV has two sets of tblgen files to run
    inline for (Backend.base_actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.base_td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    inline for (Backend.gisel_actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.gisel_td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMRISCVInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMRISCVDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            info,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMRISCVDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            desc,
            info,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMRISCVAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            desc,
            info,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/RISCV/Disassembler/CMakeLists.txt
    const mca_root = self.llvm.root.path(b, Backend.mca_root);
    const mca = self.createLLVMLibrary(.{
        .name = "LLVMRISCVTargetMCA",
        .cxx_source_files = .{
            .root = mca_root,
            .files = &Backend.mca_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            mca_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.analyzer,
            self.target_artifacts.machine_code.parser,
            desc,
            info,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMRISCVCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            mca,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.transforms.ipo,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
            self.target_artifacts.transforms.vectorize,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/CMakeLists.txt
fn compileWebAssembly(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.WebAssembly;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/Utils/CMakeLists.txt
    const utils_root = self.llvm.root.path(b, Backend.utils_root);
    const utils = self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyUtils",
        .cxx_source_files = .{
            .root = utils_root,
            .files = &Backend.utils_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            utils_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            desc,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/WebAssembly/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMWebAssemblyCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            utils,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/CMakeLists.txt
fn compileXtensa(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.Xtensa;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMXtensaInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMXtensaDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMXtensaDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMXtensaAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
            desc,
            info,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/Xtensa/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMXtensaCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            disassembler,
            asm_parser,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/PowerPC/CMakeLists.txt
fn compilePowerPC(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.PowerPC;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/PowerPC/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMPowerPCInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/PowerPC/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMPowerPCDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.types,
            self.target_artifacts.machine_code.core_lib,
            info,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/PowerPC/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMPowerPCDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.machine_code.core_lib,
            info,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/PowerPC/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMPowerPCAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            desc,
            info,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMPowerPCCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.binary_format,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.codegen.global_isel,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

/// https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/LoongArch/CMakeLists.txt
fn compileLoongArch(self: *Self) Artifact {
    const b = self.b;
    const td_files = b.addWriteFiles();

    const Backend = target_backends.LoongArch;
    const backend_root = self.llvm.root.path(b, Backend.backend_root);

    inline for (Backend.actions) |action| {
        _ = self.synthesizeHeader(td_files, .{
            .tblgen = self.full_tblgen,
            .name = action.name,
            .td_file = Backend.td_filepath,
            .instruction = .{ .actions = action.td_args },
            .virtual_path = action.name ++ ".inc",
            .extra_includes = &.{backend_root},
        });
    }

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/LoongArch/TargetInfo/CMakeLists.txt
    const info_root = self.llvm.root.path(b, Backend.info_root);
    const info = self.createLLVMLibrary(.{
        .name = "LLVMLoongArchInfo",
        .cxx_source_files = .{
            .root = info_root,
            .files = &Backend.info_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            info_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/LoongArch/MCTargetDesc/CMakeLists.txt
    const desc_root = self.llvm.root.path(b, Backend.desc_root);
    const desc = self.createLLVMLibrary(.{
        .name = "LLVMLoongArchDesc",
        .cxx_source_files = .{
            .root = desc_root,
            .files = &Backend.desc_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            desc_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.machine_code.core_lib,
            info,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/LoongArch/Disassembler/CMakeLists.txt
    const disassembler_root = self.llvm.root.path(b, Backend.disassembler_root);
    const disassembler = self.createLLVMLibrary(.{
        .name = "LLVMLoongArchDisassembler",
        .cxx_source_files = .{
            .root = disassembler_root,
            .files = &Backend.disassembler_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            disassembler_root,
            td_files.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.disassembler,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/LoongArch/AsmParser/CMakeLists.txt
    const asm_parser_root = self.llvm.root.path(b, Backend.asm_parser_root);
    const asm_parser = self.createLLVMLibrary(.{
        .name = "LLVMLoongArchAsmParser",
        .cxx_source_files = .{
            .root = asm_parser_root,
            .files = &Backend.asm_parser_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            asm_parser_root,
            td_files.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.machine_code.parser,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/Target/AArch64/CMakeLists.txt
    return self.createLLVMLibrary(.{
        .name = "LLVMLoongArchCodeGen",
        .cxx_source_files = .{
            .root = backend_root,
            .files = &Backend.backend_sources,
        },
        .additional_include_paths = &.{
            backend_root,
            td_files.getDirectory(),
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
            self.target_artifacts.target_backends.parser_gen.getDirectory(),
        },
        .link_libraries = &.{
            desc,
            info,
            disassembler,
            asm_parser,
            self.target_artifacts.analysis,
            self.target_artifacts.codegen.asm_printer,
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.codegen.types,
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.transforms.scalar,
            self.target_artifacts.codegen.selection_dag,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.transforms.utils,
        },
    });
}

fn compileExecutionEngine(self: *const Self) TargetArtifacts.ExecutionEngine {
    const b = self.b;
    var exe_engine: TargetArtifacts.ExecutionEngine = .{};

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/RuntimeDyld/CMakeLists.txt
    const runtime_dyld_root = self.llvm.root.path(b, execution_engine.runtime_dyld_root);
    exe_engine.runtime_dyld = self.createLLVMLibrary(.{
        .name = "LLVMRuntimeDyld",
        .cxx_source_files = .{
            .root = runtime_dyld_root,
            .files = &execution_engine.runtime_dyld_sources,
        },
        .additional_include_paths = &.{
            runtime_dyld_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Orc/Shared/CMakeLists.txt
    const orc_shared_root = self.llvm.root.path(b, execution_engine.orc_shared_root);
    exe_engine.orc.shared = self.createLLVMLibrary(.{
        .name = "LLVMOrcShared",
        .cxx_source_files = .{
            .root = orc_shared_root,
            .files = &execution_engine.orc_shared_sources,
        },
        .additional_include_paths = &.{
            orc_shared_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{self.target_artifacts.support},
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Orc/TargetProcess/CMakeLists.txt
    const orc_target_process_root = self.llvm.root.path(b, execution_engine.orc_target_process_root);
    exe_engine.orc.target_process = self.createLLVMLibrary(.{
        .name = "LLVMOrcTargetProcess",
        .cxx_source_files = .{
            .root = orc_target_process_root,
            .files = &execution_engine.orc_target_process_sources,
        },
        .additional_include_paths = &.{orc_target_process_root},
        .link_libraries = &.{
            exe_engine.orc.shared,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
        .bundle_compiler_rt = true,
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/JITLink/CMakeLists.txt
    const jit_link_td = self.generateTblgenInc(.{
        .tblgen = self.full_tblgen,
        .name = "COFFOptions",
        .td_file = execution_engine.jit_link_root ++ "COFFOptions.td",
        .instruction = .{ .action = "-gen-opt-parser-defs" },
    });

    const jit_link_root = self.llvm.root.path(b, execution_engine.jit_link_root);
    exe_engine.jit_link = self.createLLVMLibrary(.{
        .name = "LLVMJITLink",
        .cxx_source_files = .{
            .root = jit_link_root,
            .files = &execution_engine.jit_link_sources,
        },
        .additional_include_paths = &.{
            jit_link_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
            jit_link_td.dirname(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.object,
            self.target_artifacts.option,
            exe_engine.orc.target_process,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/CMakeLists.txt
    exe_engine.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMExecutionEngine",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, execution_engine.root),
            .files = &execution_engine.base_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.core,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.object,
            exe_engine.orc.target_process,
            exe_engine.runtime_dyld,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Orc/CMakeLists.txt
    exe_engine.orc.core_lib = self.createLLVMLibrary(.{
        .name = "LLVMOrcJIT",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, execution_engine.orc_root),
            .files = &execution_engine.orc_base_sources,
        },
        .additional_include_paths = &.{
            self.target_artifacts.intrinsics_gen.getDirectory(),
            self.minimal_artifacts.gen_vt.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.core,
            exe_engine.core_lib,
            exe_engine.jit_link,
            self.target_artifacts.object,
            exe_engine.orc.shared,
            exe_engine.orc.target_process,
            self.target_artifacts.windows_support.driver,
            self.target_artifacts.machine_code.core_lib,
            self.target_artifacts.passes,
            exe_engine.runtime_dyld,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
            self.target_artifacts.target_backends.parser,
            self.target_artifacts.text_api.core_lib,
            self.target_artifacts.transforms.utils,

            self.target_artifacts.analysis,
            self.target_artifacts.bitcode.reader,
            self.target_artifacts.bitcode.writer,
        },
        .bundle_compiler_rt = true,
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Interpreter/CMakeLists.txt
    const interpreter_root = self.llvm.root.path(b, execution_engine.interpreter_root);
    exe_engine.interpreter = self.createLLVMLibrary(.{
        .name = "LLVMInterpreter",
        .cxx_source_files = .{
            .root = interpreter_root,
            .files = &execution_engine.interpreter_sources,
        },
        .additional_include_paths = &.{
            interpreter_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.codegen.core_lib,
            self.target_artifacts.core,
            exe_engine.core_lib,
            self.target_artifacts.support,
        },
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Orc/Debugging/CMakeLists.txt
    exe_engine.orc.debugging = self.createLLVMLibrary(.{
        .name = "LLVMOrcDebugging",
        .cxx_source_files = .{
            .root = self.llvm.root.path(b, execution_engine.orc_debug_root),
            .files = &execution_engine.orc_debug_sources,
        },
        .additional_include_paths = &.{self.target_artifacts.intrinsics_gen.getDirectory()},
        .link_libraries = &.{
            self.target_artifacts.binary_format,
            self.target_artifacts.debug_info.dwarf.core_lib,
            exe_engine.jit_link,
            exe_engine.orc.core_lib,
            exe_engine.orc.shared,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.parser,
        },
        .bundle_compiler_rt = true,
    });

    // https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/llvm/lib/ExecutionEngine/Interpreter/CMakeLists.txt
    const mc_jit_root = self.llvm.root.path(b, execution_engine.mc_jit_root);
    exe_engine.mc_jit = self.createLLVMLibrary(.{
        .name = "LLVMMCJIT",
        .cxx_source_files = .{
            .root = mc_jit_root,
            .files = &execution_engine.mc_jit_sources,
        },
        .additional_include_paths = &.{
            mc_jit_root,
            self.target_artifacts.intrinsics_gen.getDirectory(),
        },
        .link_libraries = &.{
            self.target_artifacts.core,
            exe_engine.core_lib,
            self.target_artifacts.object,
            exe_engine.runtime_dyld,
            self.target_artifacts.support,
            self.target_artifacts.target_backends.core_lib,
        },
    });

    return exe_engine;
}

/// Compiles tblgen (for the host system only)
fn compileTblgen(self: *const Self, config: struct {
    support_lib: Artifact,
    minimal: bool,
}) Artifact {
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
            .files = &tblgen.common_sources,
            .flags = &common_llvm_cxx_flags,
        });

        mod.addCSourceFiles(.{
            .root = self.llvm.root.path(b, tblgen.utils_root),
            .files = &tblgen.utils_sources,
            .flags = &common_llvm_cxx_flags,
        });
    }

    mod.addIncludePath(self.llvm.llvm_include);
    mod.addIncludePath(self.llvm.root.path(b, tblgen.utils_root));
    mod.linkLibrary(config.support_lib);

    return b.addExecutable(.{
        .name = if (config.minimal) "LLVMTableGenMinimal" else "LLVMTableGen",
        .root_module = mod,
    });
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
    tblgen: Artifact,
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
    tblgen: Artifact,
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

fn compileDeps(self: *const Self, platform: Platform) !Dependencies {
    const target = switch (platform) {
        .host => self.b.graph.host,
        .target => self.target,
    };

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
/// https://github.com/allyourcodebase/zlib
fn compileZlib(self: *const Self, target: std.Build.ResolvedTarget) !Dependencies.Dependency {
    const b = self.b;
    const upstream = b.dependency("zlib", .{});
    const root = upstream.path(".");
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });

    var flags: std.ArrayList([]const u8) = .empty;
    try flags.appendSlice(b.allocator, &.{ "-std=c11", "-D_REENTRANT" });
    if (target.result.os.tag != .windows) {
        try flags.appendSlice(b.allocator, &.{ "-DHAVE_UNISTD_H", "-DHAVE_SYS_TYPES_H" });
    } else {
        try flags.append(b.allocator, "-DWIN32");
    }

    mod.addCSourceFiles(.{
        .root = root,
        .files = &dependencies.zlib,
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
/// https://github.com/allyourcodebase/libxml2
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
        .files = &dependencies.libxml2,
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
/// https://github.com/allyourcodebase/zstd
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

    // Compression & Decompression
    mod.addCSourceFiles(.{
        .root = lib_path,
        .files = &dependencies.zstd,
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

/// Returns all artifacts for the target platform
pub fn allTargetArtifacts(self: *const Self) []Artifact {
    var all_artifacts: std.ArrayList(Artifact) = .empty;
    all_artifacts.appendSlice(self.b.allocator, &.{
        self.target_artifacts.deps.zlib.artifact,
        self.target_artifacts.deps.libxml2.artifact,
        self.target_artifacts.deps.zstd.artifact,
        self.target_artifacts.demangle,
        self.target_artifacts.support,
        self.target_artifacts.bitstream_reader,
        self.target_artifacts.binary_format,
        self.target_artifacts.remarks,
        self.target_artifacts.core,
        self.target_artifacts.bitcode.reader,
        self.target_artifacts.bitcode.writer,
        self.target_artifacts.machine_code.core_lib,
        self.target_artifacts.machine_code.disassembler,
        self.target_artifacts.machine_code.parser,
        self.target_artifacts.machine_code.analyzer,
        self.target_artifacts.asm_parser,
        self.target_artifacts.ir_reader,
        self.target_artifacts.text_api.core_lib,
        self.target_artifacts.text_api.binary_reader,
        self.target_artifacts.object,
        self.target_artifacts.object_yaml,
        self.target_artifacts.debug_info.btf,
        self.target_artifacts.debug_info.code_view,
        self.target_artifacts.debug_info.dwarf.core_lib,
        self.target_artifacts.debug_info.dwarf.low_level,
        self.target_artifacts.debug_info.gsym,
        self.target_artifacts.debug_info.logical_view,
        self.target_artifacts.debug_info.msf,
        self.target_artifacts.debug_info.pdb,
        self.target_artifacts.debug_info.symbolize,
        self.target_artifacts.profile_data.coverage,
        self.target_artifacts.profile_data.core_lib,
        self.target_artifacts.analysis,
        self.target_artifacts.codegen.types,
        self.target_artifacts.codegen.data,
        self.target_artifacts.codegen.core_lib,
        self.target_artifacts.codegen.selection_dag,
        self.target_artifacts.codegen.global_isel,
        self.target_artifacts.codegen.asm_printer,
        self.target_artifacts.codegen.mir_parser,
        self.target_artifacts.sandbox_ir,
        self.target_artifacts.frontend.atomic,
        self.target_artifacts.frontend.directive,
        self.target_artifacts.frontend.driver,
        self.target_artifacts.frontend.hlsl,
        self.target_artifacts.frontend.open_acc,
        self.target_artifacts.frontend.open_mp,
        self.target_artifacts.frontend.offloading,
        self.target_artifacts.transforms.utils,
        self.target_artifacts.transforms.instrumentation,
        self.target_artifacts.transforms.aggressive_inst_combine,
        self.target_artifacts.transforms.inst_combine,
        self.target_artifacts.transforms.scalar,
        self.target_artifacts.transforms.ipo,
        self.target_artifacts.transforms.vectorize,
        self.target_artifacts.transforms.obj_carc,
        self.target_artifacts.transforms.coroutines,
        self.target_artifacts.transforms.cf_guard,
        self.target_artifacts.transforms.hip_std_par,
        self.target_artifacts.linker,
        self.target_artifacts.passes,
        self.target_artifacts.ir_printer,
        self.target_artifacts.option,
        self.target_artifacts.obj_copy,
        self.target_artifacts.dwarf_linker.core_lib,
        self.target_artifacts.dwarf_linker.classic,
        self.target_artifacts.dwarf_linker.parallel,
        self.target_artifacts.dwp,
        self.target_artifacts.file_check,
        self.target_artifacts.extensions,
        self.target_artifacts.lto,
        self.target_artifacts.xray,
        self.target_artifacts.windows_support.driver,
        self.target_artifacts.windows_support.manifest,
        self.target_artifacts.tool_drivers.lib,
        self.target_artifacts.tool_drivers.dlltool,
        self.target_artifacts.target_backends.core_lib,
        self.target_artifacts.target_backends.parser,
        self.target_artifacts.target_backends.x86,
        self.target_artifacts.target_backends.aarch64,
        self.target_artifacts.target_backends.arm,
        self.target_artifacts.target_backends.riscv,
        self.target_artifacts.target_backends.wasm,
        self.target_artifacts.target_backends.xtensa,
        self.target_artifacts.target_backends.powerpc,
        self.target_artifacts.target_backends.loong_arch,
        self.target_artifacts.execution_engine.core_lib,
        self.target_artifacts.execution_engine.interpreter,
        self.target_artifacts.execution_engine.jit_link,
        self.target_artifacts.execution_engine.mc_jit,
        self.target_artifacts.execution_engine.orc.core_lib,
        self.target_artifacts.execution_engine.orc.debugging,
        self.target_artifacts.execution_engine.orc.shared,
        self.target_artifacts.execution_engine.orc.target_process,
        self.target_artifacts.execution_engine.runtime_dyld,
    }) catch @panic("OOM");
    return all_artifacts.items;
}

pub fn allIncludePaths(self: *const Self) struct {
    includes: []std.Build.LazyPath,
    config_headers: []*std.Build.Step.ConfigHeader,
} {
    var all_includes: std.ArrayList(std.Build.LazyPath) = .empty;
    all_includes.appendSlice(self.b.allocator, &.{
        self.llvm.llvm_include,
        self.minimal_artifacts.gen_vt.getDirectory(),
        self.target_artifacts.intrinsics_gen.getDirectory(),
        self.target_artifacts.frontend.gen_files.getDirectory(),
        self.target_artifacts.target_backends.parser_gen.getDirectory(),
    }) catch @panic("OOM");

    var all_config_headers: std.ArrayList(*std.Build.Step.ConfigHeader) = .empty;
    all_config_headers.append(self.b.allocator, self.llvm.vcs_revision) catch @panic("OOM");

    return .{
        .includes = all_includes.items,
        .config_headers = all_config_headers.items,
    };
}
