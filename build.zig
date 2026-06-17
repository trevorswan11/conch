const std = @import("std");
const builtin = @import("builtin");
const zon = @import("build.zig.zon");

const Dependency = @import("third-party/Dependency.zig");
const cppcheck = @import("third-party/cppcheck.zig");
const libarchive = @import("third-party/libarchive.zig");
const fmt = @import("third-party/fmt.zig");
const catch2 = @import("third-party/catch2.zig");
const replxx = @import("third-party/replxx.zig");

const KcovBuilder = @import("third-party/kcov/KcovBuilder.zig");

const LLVMBuilder = @import("third-party/llvm/LLVMBuilder.zig");
const ClangBuilder = @import("third-party/llvm/ClangBuilder.zig");
const LLDBuilder = @import("third-party/llvm/LLDBuilder.zig");

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const llvm: *LLVMBuilder = .init(b);
    const clang: *ClangBuilder = .init(llvm);
    const cdb_gen: *CDBGenerator = .init(b);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
        "-Wconversion",
        "-Wshadow",
        "-Wno-gnu-statement-expression",
        "-Wno-gnu-statement-expression-from-macro-expansion",
        "-DMAGIC_ENUM_RANGE_MAX=255",
        "-DREPLXX_STATIC",
    });
    const dist_flags: []const []const u8 = &.{ "-DNDEBUG", "-DGHOTI_DIST" };

    var package_flags = try compiler_flags.clone(b.allocator);
    try package_flags.appendSlice(b.allocator, dist_flags);

    if (b.option(bool, "profile", "Enable chromium tracing") orelse false) {
        try compiler_flags.append(b.allocator, "-DGHOTI_PROFILE");
    }

    try compiler_flags.appendSlice(b.allocator, &.{
        "-gen-cdb-fragment-path",
        b.cache_root.join(b.allocator, &.{CDBGenerator.cdb_frags_dirname}) catch @panic("OOM"),
    });

    switch (optimize) {
        .Debug => try compiler_flags.appendSlice(b.allocator, &.{ "-g", "-DGHOTI_DEBUG" }),
        .ReleaseSafe => try compiler_flags.appendSlice(b.allocator, &.{"-DGHOTI_RELEASE"}),
        .ReleaseFast, .ReleaseSmall => try compiler_flags.appendSlice(b.allocator, dist_flags),
    }

    const install_tests_only = b.option(
        bool,
        "install-tests-only",
        "Install tests without running them (default: false)",
    ) orelse false;

    const site_builder = try SiteBuilder.init(b, optimize);
    if (site_builder) |site| try site.build();

    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;
    const artifacts = try addArtifacts(b, .{
        .optimize = optimize,
        .llvm = llvm,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
        .install_tests_only = install_tests_only,
        .site_builder = site_builder,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    clang.build();
    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .clang = clang,
        .cppcheck = artifacts.cppcheck.?,
        .site_builder = site_builder,
    });

    try addPackageStep(b, .{
        .llvm = llvm,
        .cxx_flags = package_flags.items,
    });

    if (artifacts.tests) |tests| try addCoverageStep(b, tests);
}

const ProjectPaths = struct {
    const Project = struct {
        inc: []const u8,
        src: []const u8,
        tests: []const u8,

        pub fn files(self: *const Project, b: *std.Build) ![][]const u8 {
            return std.mem.concat(b.allocator, []const u8, &.{
                try collectFiles(b, self.inc, .{ .allowed_extensions = &.{".hh"} }),
                try collectFiles(b, self.src, .{ .allowed_extensions = &.{".cc"} }),
                try collectFiles(b, self.tests, .{ .allowed_extensions = &.{ ".hh", ".cc" } }),
            });
        }
    };

    const compiler: Project = .{
        .inc = "lib/compiler/include/",
        .src = "lib/compiler/src/",
        .tests = "lib/compiler/tests/",
    };

    const driver: Project = .{
        .inc = "lib/driver/include/",
        .src = "lib/driver/src/",
        .tests = "lib/driver/tests/",
    };
    const ghoti = "ghoti/main.cc";

    const support: Project = .{
        .inc = "lib/support/include/",
        .src = "lib/support/src/",
        .tests = "lib/support/tests/",
    };

    const stdlib = "lib/std/";
    const compressor = "tools/compressor/";
    const harness = "tools/harness/";

    pub fn collectCXXToolingFiles(b: *std.Build) ![]const []const u8 {
        return std.mem.concat(b.allocator, []const u8, &.{
            try compiler.files(b),
            try driver.files(b),
            try support.files(b),
            try collectFiles(b, harness, .{ .allowed_extensions = &.{".cc"} }),
        });
    }

    const site = "site/";
    const third_party = "third-party/";
};

const ExecutableBehavior = union(enum) {
    // Meant for user facing potentially runnable commands
    installable: struct {
        cmd_name: []const u8,
        cmd_desc: []const u8,
        install_dir: ?[]const u8 = null,
        install_only: bool = false,
    },

    // Meant for internal tools and intermediate artifacts
    standalone: void,

    pub fn installArtifact(
        b: *std.Build,
        artifact: *std.Build.Step.Compile,
        parent_step: *std.Build.Step,
        install_dir: ?[]const u8,
        install_only: bool,
    ) ?*std.Build.Step.Run {
        var runner: ?*std.Build.Step.Run = null;
        if (!install_only) {
            runner = b.addRunArtifact(artifact);
            runner.?.step.dependOn(b.getInstallStep());
            parent_step.dependOn(&runner.?.step);
        }

        if (install_dir) |override| {
            const install = b.addInstallArtifact(artifact, .{
                .dest_dir = .{
                    .override = .{ .custom = override },
                },
            });
            parent_step.dependOn(&install.step);
        }
        return runner;
    }
};

const TestArtifacts = struct {
    const WebserverTests = struct {
        run: *std.Build.Step.Run,
        install: *std.Build.Step.InstallDir,
    };

    harness_tests: *std.Build.Step.Compile = undefined,
    support_tests: *std.Build.Step.Compile = undefined,
    compiler_tests: *std.Build.Step.Compile = undefined,
    driver_tests: *std.Build.Step.Compile = undefined,
    webserver_tests: ?WebserverTests = null,

    pub fn configure(
        self: *const TestArtifacts,
        b: *std.Build,
        cdb_steps: ?*std.ArrayList(*std.Build.Step),
        install_dir: ?[]const u8,
        install_only: bool,
    ) !void {
        if (cdb_steps) |cdb| {
            try cdb.append(b.allocator, &self.support_tests.step);
            try cdb.append(b.allocator, &self.compiler_tests.step);
            try cdb.append(b.allocator, &self.driver_tests.step);
        }

        const artifacts = [_]*std.Build.Step.Compile{
            self.harness_tests,
            self.support_tests,
            self.compiler_tests,
            self.driver_tests,
        };

        const test_step = b.step("test", "Run all unit tests");
        for (artifacts) |artifact| {
            _ = ExecutableBehavior.installArtifact(
                b,
                artifact,
                test_step,
                install_dir,
                install_only,
            );
        }

        if (self.webserver_tests) |webserver_tests| {
            if (install_only) {
                webserver_tests.run.addArg("-c");
            }
            test_step.dependOn(&webserver_tests.install.step);
        }
    }
};

const version_str = zon.version;
const version = std.SemanticVersion.parse(version_str) catch @compileError("Malformed version");

fn makeConfigHeader(b: *std.Build, target: std.Build.ResolvedTarget) *std.Build.Step.ConfigHeader {
    const git_hash = std.mem.trimEnd(u8, b.run(&.{ "git", "rev-parse", "HEAD" }), " \r\n");
    var out_code: u8 = undefined;
    const git_tag_raw = b.runAllowFail(&.{ "git", "describe", "--tags", "--abbrev=0" }, &out_code, .ignore) catch "";
    const git_tag = std.mem.trimEnd(u8, git_tag_raw, " \r\n");

    return b.addConfigHeader(.{}, .{
        .GHOTI_VERSION_STR = version_str,
        .GHOTI_VERSION_MAJOR = @as(i64, version.major),
        .GHOTI_VERSION_MINOR = @as(i64, version.minor),
        .GHOTI_VERSION_PATCH = @as(i64, version.patch),
        .GHOTI_VERSION_PRE = version.pre orelse "",
        .GHOTI_GIT_INFO = b.fmt("git-{s}{s}{s}", .{ git_hash, if (git_tag_raw.len == 0) "" else "-", git_tag }),
        .GHOTI_WINDOWS = target.result.os.tag == .windows,
        .GHOTI_LINUX = target.result.os.tag == .linux,
        .GHOTI_APPLE = target.result.os.tag == .macos,
    });
}

fn addArtifacts(b: *std.Build, config: struct {
    target: ?std.Build.ResolvedTarget = null,
    optimize: std.builtin.OptimizeMode,
    llvm: *LLVMBuilder,
    cxx_flags: []const []const u8,
    cdb_steps: ?*std.ArrayList(*std.Build.Step),
    behavior: ?ExecutableBehavior = null,
    auto_install: bool = true,
    packaging: bool = false,
    install_tests_only: bool = true,
    site_builder: ?*SiteBuilder = null,
}) !struct {
    libsupport: *std.Build.Step.Compile,
    libcompiler: *std.Build.Step.Compile,
    libdriver: *std.Build.Step.Compile,
    ghoti: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
} {
    const target = config.target orelse b.graph.host;
    const config_h = makeConfigHeader(b, target);
    const building_for_host = config.target == null;

    const magic_enum = b.dependency("magic_enum", .{});
    const magic_enum_inc = magic_enum.path("include");

    const unordered_dense = b.dependency("unordered_dense", .{});
    const unordered_dense_inc = unordered_dense.path("include");

    const gsl = b.dependency("gsl", .{});
    const gsl_inc = gsl.path("include");

    const cli11 = b.dependency("cli11", .{});
    const cli11_inc = cli11.path("include");

    const system_includes = [_]std.Build.LazyPath{
        magic_enum_inc,
        unordered_dense_inc,
        gsl_inc,
        cli11_inc,
    };

    const dep_config: Dependency.Config = .{
        .target = target,
        .optimize = config.optimize,
    };

    const fmt_dep = fmt.build(b, dep_config);
    const replxx_dep = replxx.build(b, dep_config);

    // Shared core functionality
    const libsupport = b.addLibrary(.{
        .name = "support",
        .root_module = createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{b.path(ProjectPaths.support.inc)},
            .system_include_paths = &system_includes,
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.support.src, .{}),
                .flags = config.cxx_flags,
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{ fmt_dep.artifact, replxx_dep.artifact },
        }),
    });
    if (config.auto_install) b.installArtifact(libsupport);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libsupport.step);

    // LLVM is compiled from source because I like burning compute or something
    config.llvm.build(.{
        .target = target,
        .behavior = if (config.packaging)
            .package
        else
            .{ .allow_kaleidoscope = config.auto_install },
    });

    // The compiler's implementation & static library
    const libcompiler = b.addLibrary(.{
        .name = "compiler",
        .root_module = createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
            },
            .system_include_paths = &system_includes,
            .config_headers = &.{config_h},
            .link_libraries = &.{ libsupport, fmt_dep.artifact },
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.compiler.src, .{}),
                .flags = config.cxx_flags,
            },
        }),
    });
    if (config.auto_install) b.installArtifact(libcompiler);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcompiler.step);

    // The user-facing library
    const libdriver = b.addLibrary(.{
        .name = "driver",
        .root_module = createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.driver.inc),
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
            },
            .system_include_paths = &system_includes,
            .config_headers = &.{config_h},
            .link_libraries = &.{ libcompiler, fmt_dep.artifact, replxx_dep.artifact },
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.driver.src, .{}),
                .flags = config.cxx_flags,
            },
        }),
    });
    if (config.auto_install) b.installArtifact(libdriver);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libdriver.step);

    // The shippable executable
    const ghoti = createExecutable(b, .{
        .target = target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.driver.inc),
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.support.inc),
        },
        .cxx = .{
            .files = &.{ProjectPaths.ghoti},
            .flags = config.cxx_flags,
        },
        .system_include_paths = &system_includes,
        .link_libraries = &.{ libdriver, fmt_dep.artifact, replxx_dep.artifact },
    }, .{
        .name = "ghoti",
        .behavior = config.behavior orelse .{
            .installable = .{
                .cmd_name = "run",
                .cmd_desc = "Run ghoti with provided command line arguments",
            },
        },
    });
    if (config.auto_install) b.installArtifact(ghoti);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &ghoti.step);

    var tests: ?TestArtifacts = null;
    if (building_for_host) {
        const test_install_dir: ?[]const u8 = if (config.auto_install) "tests" else null;
        const harness_main = b.path(ProjectPaths.harness ++ "main.zig");
        const catch2_dep = catch2.build(b, .{
            .target = target,
            .optimize = config.optimize,
        });

        // The test harness has standalone tests of its own
        const harness = b.addTest(.{
            .name = "harness",
            .root_module = b.createModule(.{
                .root_source_file = harness_main,
                .optimize = config.optimize,
                .target = target,
                .link_libc = true,
            }),
        });

        const harness_step = b.step("test-harness", "Build/run test harness' tests");
        _ = ExecutableBehavior.installArtifact(
            b,
            harness,
            harness_step,
            test_install_dir,
            config.install_tests_only,
        );

        // Support's tests depend on the test runner but not LLVM
        const support_tests = createExecutable(b, .{
            .target = target,
            .optimize = config.optimize,
            .zig_main = harness_main,
            .include_paths = &.{
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.support.tests),
            },
            .system_include_paths = &system_includes,
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.support.tests, .{
                    .extra_files = &.{ProjectPaths.harness ++ "runner.cc"},
                }),
                .flags = config.cxx_flags,
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{ libsupport, catch2_dep.artifact, fmt_dep.artifact },
        }, .{
            .name = "support",
            .behavior = config.behavior orelse .{
                .installable = .{
                    .cmd_name = "test-support",
                    .cmd_desc = "Build/run support's unit tests",
                    .install_dir = test_install_dir,
                    .install_only = config.install_tests_only,
                },
            },
        });

        const compiler_tests = createExecutable(b, .{
            .target = target,
            .optimize = config.optimize,
            .zig_main = harness_main,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.compiler.tests),
            },
            .system_include_paths = &system_includes,
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.compiler.tests, .{
                    .extra_files = &.{ProjectPaths.harness ++ "runner.cc"},
                }),
                .flags = config.cxx_flags,
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{ libcompiler, catch2_dep.artifact, fmt_dep.artifact },
        }, .{
            .name = "compiler",
            .behavior = config.behavior orelse .{
                .installable = .{
                    .cmd_name = "test-compiler",
                    .cmd_desc = "Build/run compiler unit tests",
                    .install_dir = test_install_dir,
                    .install_only = config.install_tests_only,
                },
            },
        });

        const driver_tests = createExecutable(b, .{
            .target = target,
            .optimize = config.optimize,
            .zig_main = b.path(ProjectPaths.harness ++ "main.zig"),
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.driver.inc),
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.driver.tests),
            },
            .system_include_paths = &system_includes,
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.driver.tests, .{
                    .extra_files = &.{ProjectPaths.harness ++ "runner.cc"},
                }),
                .flags = config.cxx_flags,
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{
                libcompiler,
                libdriver,
                catch2_dep.artifact,
                fmt_dep.artifact,
                replxx_dep.artifact,
            },
        }, .{
            .name = "driver",
            .behavior = config.behavior orelse .{
                .installable = .{
                    .cmd_name = "test-driver",
                    .cmd_desc = "Build/run the driver's unit tests",
                    .install_dir = test_install_dir,
                    .install_only = config.install_tests_only,
                },
            },
        });

        var webserver_tests: ?TestArtifacts.WebserverTests = null;
        if (config.site_builder) |site| {
            const ws_run = b.addSystemCommand(&.{ site.go_exe_path, "test", "./...", "-o" });
            ws_run.setCwd(site.site_path);
            const ws_tests = ws_run.addOutputDirectoryArg("webserver");
            const ws_install = b.addInstallDirectory(.{
                .source_dir = ws_tests,
                .install_dir = .{ .custom = "tests" },
                .install_subdir = "webserver",
            });
            ws_run.has_side_effects = true;
            ws_run.step.dependOn(&site.main_builder.step);

            const step = b.step("test-webserver", "Build/run the webserver's tests");
            step.dependOn(&ws_install.step);

            webserver_tests = .{
                .run = ws_run,
                .install = ws_install,
            };
        }

        tests = .{
            .harness_tests = harness,
            .support_tests = support_tests,
            .compiler_tests = compiler_tests,
            .driver_tests = driver_tests,
            .webserver_tests = webserver_tests,
        };
        try tests.?.configure(b, config.cdb_steps, test_install_dir, config.install_tests_only);
    }

    const cppcheck_dep: ?Dependency = if (building_for_host) try cppcheck.build(b, .{
        .target = target,
        .optimize = .ReleaseFast,
    }) else null;

    return .{
        .libsupport = libsupport,
        .libcompiler = libcompiler,
        .libdriver = libdriver,
        .ghoti = ghoti,
        .tests = tests,
        .cppcheck = if (cppcheck_dep) |dep| dep.artifact else null,
    };
}

const CreateModuleConfig = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    zig_main: ?std.Build.LazyPath = null,
    include_paths: ?[]const std.Build.LazyPath = null,
    system_include_paths: ?[]const std.Build.LazyPath = null,
    config_headers: ?[]const *std.Build.Step.ConfigHeader = null,
    source_root: ?std.Build.LazyPath = null,
    link_libraries: ?[]const *std.Build.Step.Compile = null,
    system_libraries: ?struct {
        search_paths: []const std.Build.LazyPath,
        libs: []const []const u8,
    } = null,
    imports: ?[]const struct {
        name: []const u8,
        module: *std.Build.Module,
    } = null,
    cxx: ?struct {
        files: []const []const u8,
        flags: []const []const u8,
    } = null,
};

fn createModule(b: *std.Build, config: CreateModuleConfig) *std.Build.Module {
    const mod = b.createModule(.{
        .root_source_file = config.zig_main,
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    if (config.include_paths) |include_paths| for (include_paths) |inc_path| {
        mod.addIncludePath(inc_path);
    };

    if (config.system_include_paths) |system_includes| for (system_includes) |inc_path| {
        mod.addSystemIncludePath(inc_path);
    };

    if (config.config_headers) |config_headers| for (config_headers) |header| {
        mod.addConfigHeader(header);
    };

    if (config.link_libraries) |link_libraries| for (link_libraries) |lib| {
        mod.linkLibrary(lib);
    };

    if (config.cxx) |cxx| mod.addCSourceFiles(.{
        .root = config.source_root,
        .files = cxx.files,
        .flags = cxx.flags,
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

    if (config.imports) |imports| for (imports) |import| {
        mod.addImport(import.name, import.module);
    };

    return mod;
}

fn createExecutable(
    b: *std.Build,
    module_config: CreateModuleConfig,
    executable_config: struct {
        name: []const u8,
        behavior: ExecutableBehavior = .standalone,
    },
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = executable_config.name,
        .root_module = createModule(b, module_config),
    });

    switch (executable_config.behavior) {
        .installable => |config| {
            const step = b.step(config.cmd_name, config.cmd_desc);
            if (ExecutableBehavior.installArtifact(
                b,
                exe,
                step,
                config.install_dir,
                config.install_only,
            )) |run| {
                if (b.args) |args| {
                    run.addArgs(args);
                }
            }
        },
        .standalone => {},
    }

    return exe;
}

const CDBGenerator = struct {
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

    pub fn init(b: *std.Build) *CDBGenerator {
        const self = b.allocator.create(CDBGenerator) catch @panic("OOM");
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

    pub fn getCdbPath(self: *const CDBGenerator) std.Build.LazyPath {
        return .{ .generated = .{ .file = &self.output_file } };
    }

    fn generateCdb(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *CDBGenerator = @fieldParentPtr("step", step);

        const b = step.owner;
        const allocator = b.allocator;
        const io = b.graph.io;
        const cache_root = b.cache_root.handle;

        self.output_file.path = b.cache_root.join(b.allocator, &.{cdb_filename}) catch @panic("OOM");
        try cache_root.createDirPath(io, cdb_frags_dirname);
        var newest_frags: std.StringHashMap(FragInfo) = .init(allocator);

        var dir = try cache_root.openDir(io, cdb_frags_dirname, .{ .iterate = true });
        defer dir.close(io);
        var dir_iter = dir.iterate();

        // The frags balloon like crazy so cleaning up proactively is needed
        var old_frags: std.ArrayList([]const u8) = .empty;

        // Hashed updates are generated by the compiler, so grab the most recent for the cdb
        const file_buf = try allocator.alloc(u8, 64 * 1024);
        while (try dir_iter.next(io)) |entry| {
            if (entry.kind != .file) continue;
            const entry_name = b.dupe(entry.name);
            const stat = try dir.statFile(io, entry_name, .{});

            const entry_contents = try dir.readFile(io, entry_name, file_buf);
            const trimmed = std.mem.trimEnd(u8, entry_contents, ",\n\r\t");
            const parsed: ParsedCdbFileInfo = std.json.parseFromSlice(
                CdbFileInfo,
                allocator,
                trimmed,
                .{ .ignore_unknown_fields = true },
            ) catch continue;
            const ref_path = parsed.value.file;
            const absolute_ref_path = if (std.Io.Dir.path.isAbsolute(ref_path))
                b.dupe(ref_path)
            else
                try b.build_root.join(allocator, &.{ref_path});

            // Orphaned files should be removed too
            std.Io.Dir.accessAbsolute(io, absolute_ref_path, .{}) catch {
                try old_frags.append(allocator, entry_name);
                continue;
            };

            const gop = try newest_frags.getOrPut(absolute_ref_path);
            const mtime: i128 = stat.mtime.nanoseconds;
            if (!gop.found_existing) {
                gop.value_ptr.* = .{
                    .name = entry_name,
                    .mtime = mtime,
                };
            } else {
                if (mtime > gop.value_ptr.mtime) {
                    try old_frags.append(allocator, gop.value_ptr.name);
                    gop.value_ptr.name = entry_name;
                    gop.value_ptr.mtime = mtime;
                } else {
                    try old_frags.append(allocator, entry_name);
                }
            }
        }

        for (old_frags.items) |old| {
            dir.deleteFile(io, old) catch continue;
        }

        var frag_iter = newest_frags.valueIterator();
        var first = true;
        const cdb = try cache_root.createFile(io, cdb_filename, .{});
        defer cdb.close(io);

        var cdb_buffer: [1024]u8 = undefined;
        var cdb_writer = cdb.writer(io, &cdb_buffer);
        const writer = &cdb_writer.interface;

        try writer.writeAll("[");
        while (frag_iter.next()) |info| {
            if (!first) try writer.writeAll(",\n");
            first = false;

            const fpath = b.pathJoin(&.{ cdb_frags_dirname, info.name });
            const contents = try cache_root.readFile(io, fpath, file_buf);
            const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
            try writer.writeAll(trimmed);
        }
        try writer.writeAll("]");
        try writer.flush();
    }
};

fn addTooling(b: *std.Build, config: struct {
    cdb_gen: *CDBGenerator,
    clang: *ClangBuilder,
    cppcheck: *std.Build.Step.Compile,
    site_builder: ?*SiteBuilder,
}) !void {
    const tooling_sources = try ProjectPaths.collectCXXToolingFiles(b);

    const cdb_step = b.step("cdb", "Generate " ++ CDBGenerator.cdb_filename);
    cdb_step.dependOn(&config.cdb_gen.step);
    b.getInstallStep().dependOn(&config.cdb_gen.step);

    try addFmtStep(b, .{
        .tooling_sources = tooling_sources,
        .clang = config.clang,
        .site_builder = config.site_builder,
    });

    const check_step = addStaticAnalysisStep(b, .{
        .tooling_sources = tooling_sources,
        .cppcheck = config.cppcheck,
        .cdb_gen = config.cdb_gen,
        .site_builder = config.site_builder,
    });
    check_step.dependOn(&config.cdb_gen.step);

    const cloc: *LOCCounter = .init(b);
    const cloc_step = b.step("cloc", "Count lines of code across the project");
    cloc_step.dependOn(&cloc.step);
}

fn addFmtStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    clang: *ClangBuilder,
    site_builder: ?*SiteBuilder,
}) !void {
    const clang_format = config.clang.clang_tools.clang_format;
    const zig_paths = try collectFiles(b, "tools", .{
        .allowed_extensions = &.{".zig"},
        .extra_files = &.{
            "build.zig",
            "build.zig.zon",
            ProjectPaths.site ++ "rebuild.zig",
        },
    });
    const build_fmt = b.addFmt(.{ .paths = zig_paths });
    const build_fmt_check = b.addFmt(.{ .paths = zig_paths, .check = true });

    const formatter = b.addRunArtifact(clang_format);
    formatter.addArg("-i");
    formatter.addArgs(config.tooling_sources);
    const fmt_step = b.step("fmt", "Format all project files");
    fmt_step.dependOn(&formatter.step);
    fmt_step.dependOn(&build_fmt.step);

    const fmt_check = b.addRunArtifact(clang_format);
    fmt_check.addArgs(&.{ "--dry-run", "--Werror" });
    fmt_check.addArgs(config.tooling_sources);
    const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
    fmt_check_step.dependOn(&fmt_check.step);
    fmt_check_step.dependOn(&build_fmt_check.step);

    if (config.site_builder) |site| if (site.go_fmt_path) |go_fmt| {
        const go_paths = try collectFiles(b, ProjectPaths.site, .{
            .allowed_extensions = &.{".go"},
            .dropped_extensions = &.{"_templ.go"},
        });

        const go_formatter = b.addSystemCommand(&.{ go_fmt, "-w" });
        go_formatter.addArgs(go_paths);
        fmt_step.dependOn(&go_formatter.step);

        const templ_formatter = site.addRunTempl();
        templ_formatter.setCwd(site.site_path);
        templ_formatter.addArgs(&.{ "fmt", "." });
        fmt_step.dependOn(&templ_formatter.step);

        const go_formatter_check = b.addSystemCommand(&.{ go_fmt, "-d" });
        go_formatter_check.addArgs(go_paths);
        fmt_check_step.dependOn(&go_formatter_check.step);

        const templ_formatter_check = site.addRunTempl();
        templ_formatter_check.setCwd(site.site_path);
        templ_formatter_check.addArgs(&.{ "fmt", "-fail", "." });
        fmt_check_step.dependOn(&templ_formatter_check.step);
    };
}

fn addStaticAnalysisStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    cppcheck: *std.Build.Step.Compile,
    cdb_gen: *CDBGenerator,
    site_builder: ?*SiteBuilder,
}) *std.Build.Step {
    const check_step = b.step("check", "Run static analysis on all project files");
    const cppcheck_run = b.addRunArtifact(config.cppcheck);

    const installed_cppcheck_cache_path = b.cache_root.join(b.allocator, &.{"cppcheck"}) catch @panic("OOM");
    cppcheck_run.addArg("--inline-suppr");
    cppcheck_run.addPrefixedFileArg("--project=", config.cdb_gen.getCdbPath());
    const cppcheck_cache = cppcheck_run.addPrefixedOutputDirectoryArg(
        "--cppcheck-build-dir=",
        installed_cppcheck_cache_path,
    );
    cppcheck_run.addArg("--check-level=exhaustive");
    cppcheck_run.addArgs(&.{ "--error-exitcode=1", "--enable=all" });
    cppcheck_run.addArgs(&.{
        "--suppress=*:magic_enum.hpp",
        "--suppress=*:.zig-cache/*",
        "--suppress=*:*llvm/*",
        "--suppress=*:*CLI/*",
    });

    // Other spurious warnings
    const suppressions: []const []const u8 = &.{
        "checkersReport",
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
        "functionStatic",
    };

    inline for (suppressions) |suppression| {
        cppcheck_run.addArg("--suppress=" ++ suppression);
    }

    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.support.tests));
    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.compiler.tests));
    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.driver.tests));

    const cppcheck_cache_install = b.addInstallDirectory(.{
        .source_dir = cppcheck_cache,
        .install_dir = .{ .custom = ".." },
        .install_subdir = installed_cppcheck_cache_path,
    });

    cppcheck_cache_install.step.dependOn(&config.cppcheck.step);
    check_step.dependOn(&cppcheck_cache_install.step);
    check_step.dependOn(&cppcheck_run.step);

    if (config.site_builder) |site| {
        const go_vet = b.addSystemCommand(&.{ site.go_exe_path, "vet", "./..." });
        go_vet.setCwd(site.site_path);
        go_vet.step.dependOn(&site.main_builder.step);
        check_step.dependOn(&go_vet.step);
    }
    return check_step;
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
            const ext = std.Io.Dir.path.extension(file_path)[1..];
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

        pub fn print(self: *const LOCResult, io: std.Io) !void {
            const stdout_handle = std.Io.File.stdout();
            var stdout_buf: [1024]u8 = undefined;
            var stdout_writer = stdout_handle.writer(io, &stdout_buf);
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

    const counted_extensions = [_][]const u8{
        ".cc", ".hh",    ".inc",  ".zig", ".gh",
        ".go", ".templ", ".html", ".css",
    };

    step: std.Build.Step,

    pub fn init(b: *std.Build) *LOCCounter {
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
        const io = b.graph.io;

        const counted_files = try std.mem.concat(b.allocator, []const u8, &.{
            try collectFiles(b, "lib", .{
                .allowed_extensions = &counted_extensions,
                .extra_files = &.{"build.zig"},
            }),
            try collectFiles(b, "ghoti", .{ .allowed_extensions = &counted_extensions }),
            try collectFiles(b, "tools", .{ .allowed_extensions = &counted_extensions }),
            try collectFiles(b, "site", .{ .allowed_extensions = &counted_extensions }),
        });

        const build_dir = b.build_root.handle;
        const buffer = try b.allocator.alloc(u8, 100 * 1024);
        var result: LOCResult = .init(b.allocator);

        for (counted_files) |file| {
            const contents = try build_dir.readFile(io, file, buffer);
            var it = std.mem.tokenizeAny(u8, contents, "\r\n");

            var lines: usize = 0;
            while (it.next()) |line| {
                const trimmed = std.mem.trim(u8, line, " \t\n\r");
                if (trimmed.len > 0 and !std.mem.startsWith(u8, trimmed, "//")) {
                    lines += 1;
                }
            }
            try result.logFile(file, lines);
        }

        try result.print(io);
    }
};

// Mimics `b.addRemoveDirTree` that was removed in 0.16.0
const RemoveDir = struct {
    step: std.Build.Step,
    doomed_path: std.Build.LazyPath,

    pub fn init(b: *std.Build, doomed_path: std.Build.LazyPath) *RemoveDir {
        const remove_dir = b.allocator.create(RemoveDir) catch @panic("OOM");
        remove_dir.* = .{
            .step = .init(.{
                .id = .custom,
                .name = b.fmt("RemoveDir {s}", .{doomed_path.getDisplayName()}),
                .owner = b,
                .makeFn = make,
            }),
            .doomed_path = doomed_path.dupe(b),
        };
        return remove_dir;
    }

    fn make(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *RemoveDir = @fieldParentPtr("step", step);

        const b = step.owner;
        const io = b.graph.io;

        step.clearWatchInputs();
        try step.addWatchInput(self.doomed_path);

        const full_doomed_path = try self.doomed_path.getPath4(b, step);

        b.build_root.handle.deleteTree(io, full_doomed_path.sub_path) catch |err| {
            if (b.build_root.path) |base| {
                return step.fail("unable to recursively delete path '{s}/{s}': {s}", .{
                    base, full_doomed_path.sub_path, @errorName(err),
                });
            } else {
                return step.fail("unable to recursively delete path '{s}': {s}", .{
                    full_doomed_path.sub_path, @errorName(err),
                });
            }
        };
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
    llvm: *LLVMBuilder,
    cxx_flags: []const []const u8,
}) !void {
    const libarchive_dep = libarchive.build(b, .{
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    });

    const headers = b.addTranslateC(.{
        .root_source_file = b.path(ProjectPaths.compressor ++ "c.h"),
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    });
    headers.addIncludePath(libarchive_dep.artifact.getEmittedIncludeTree());

    const compressor = createExecutable(b, .{
        .zig_main = b.path(ProjectPaths.compressor ++ "main.zig"),
        .target = b.graph.host,
        .optimize = .ReleaseFast,
        .link_libraries = &.{libarchive_dep.artifact},
        .imports = &.{
            .{
                .name = "c",
                .module = headers.createModule(),
            },
        },
    }, .{
        .name = "compressor",
        .behavior = .standalone,
    });
    Dependency.addFrameworkSearchPaths(compressor.root_module, b.graph.host);

    // Always clean up the compressed dir before packaging
    const package_parent_dirname = "package";
    const cleaner: *RemoveDir = .init(b, .{
        .cwd_relative = b.pathJoin(&.{ b.install_prefix, package_parent_dirname }),
    });
    compressor.step.dependOn(&cleaner.step);

    const package_step = b.step("package", "Package artifacts for a new release");
    package_step.dependOn(&compressor.step);

    const ArchiveBehavior = struct {
        compressor_arg: enum { zip, zst },
        file_extension: []const u8,
        skip: bool = false,
    };

    for (target_queries) |query| {
        const target = b.resolveTargetQuery(query);
        const artifacts = try addArtifacts(b, .{
            .target = target,
            .optimize = .ReleaseFast,
            .llvm = config.llvm.clone(),
            .cxx_flags = config.cxx_flags,
            .cdb_steps = null,
            .behavior = .standalone,
            .auto_install = false,
            .packaging = true,
        });
        std.debug.assert(artifacts.tests == null);
        std.debug.assert(artifacts.cppcheck == null);

        artifacts.ghoti.out_filename = blk: {
            const name = artifacts.ghoti.name;
            break :blk if (target.result.os.tag == .windows)
                b.fmt("{s}-{s}.exe", .{ name, version_str })
            else
                b.fmt("{s}-{s}", .{ name, version_str });
        };
        artifacts.ghoti.root_module.strip = true;

        const package_artifact_dirname = b.fmt("ghoti-{s}-{s}", .{
            try query.zigTriple(b.allocator),
            version_str,
        });

        const staging = b.addWriteFiles();
        const artifact_dest_path = b.fmt("{s}/{s}", .{ package_artifact_dirname, artifacts.ghoti.out_filename });
        _ = staging.addCopyFile(artifacts.ghoti.getEmittedBin(), artifact_dest_path);

        const legal_paths = [_]struct { std.Build.LazyPath, []const u8 }{
            .{ b.path("LICENSE"), "LICENSE" },
            .{ b.path("README.md"), "README.md" },
            .{ b.path(".github/CHANGELOG.md"), "CHANGELOG.md" },
        };

        for (legal_paths) |path| {
            const src, const dst = path;
            _ = staging.addCopyFile(src, b.fmt("{s}/{s}", .{ package_artifact_dirname, dst }));
        }

        // Zip is only needed on windows
        const archives = [_]ArchiveBehavior{
            .{
                .compressor_arg = .zip,
                .file_extension = "zip",
                .skip = target.result.os.tag != .windows,
            },
            .{
                .compressor_arg = .zst,
                .file_extension = "tar.zst",
            },
        };

        for (archives) |archive| {
            if (archive.skip) continue;
            const out_name = b.fmt("{s}.{s}", .{
                package_artifact_dirname,
                archive.file_extension,
            });

            const packer = b.addRunArtifact(compressor);
            packer.addArg(@tagName(archive.compressor_arg));
            const out_path = packer.addOutputFileArg(out_name);
            packer.addDirectoryArg(staging.getDirectory().path(b, package_artifact_dirname));
            package_step.dependOn(&packer.step);

            const copy = b.addInstallFileWithDir(
                out_path,
                .{ .custom = package_parent_dirname },
                out_name,
            );
            package_step.dependOn(&copy.step);
        }
    }
}

/// Adds coverage reporting on supported platforms for all test artifacts
fn addCoverageStep(b: *std.Build, tests: TestArtifacts) !void {
    const kcov = KcovBuilder.build(b, .{
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    }) orelse return;
    const reports = [_]KcovBuilder.RunKcovReport{
        try kcov.runKcov(.{
            .artifact = tests.harness_tests,
            .include_patterns = &.{ProjectPaths.harness},
        }),
        try kcov.runKcov(.{
            .artifact = tests.support_tests,
            .include_patterns = &.{ ProjectPaths.support.src, ProjectPaths.support.inc },
        }),
        try kcov.runKcov(.{
            .artifact = tests.compiler_tests,
            .include_patterns = &.{ ProjectPaths.compiler.src, ProjectPaths.compiler.inc },
        }),
        try kcov.runKcov(.{
            .artifact = tests.driver_tests,
            .include_patterns = &.{ ProjectPaths.driver.src, ProjectPaths.driver.inc },
        }),
    };

    const coverage = b.step("coverage", "Generate coverage report");
    for (reports) |report| {
        coverage.dependOn(&report.runner.step);
    }
    const merged = kcov.mergeKcovReports(&reports);

    const install_merged = b.option(
        bool,
        "install-merged",
        "install merged kcov report",
    ) orelse false;
    if (install_merged) {
        const merged_output_dirname = "merged";
        const install = b.addInstallDirectory(.{
            .source_dir = merged.output_dir,
            .install_dir = .prefix,
            .install_subdir = merged_output_dirname,
        });

        const remove: *RemoveDir = .init(b, .{
            .cwd_relative = b.pathJoin(&.{
                b.install_prefix,
                merged_output_dirname,
            }),
        });
        install.step.dependOn(&remove.step);
        coverage.dependOn(&install.step);
    }

    const curl = b.addRunArtifact(kcov.curl.execurl);
    curl.addArg("-o");
    const badge_file = curl.addOutputFileArg("coverage.svg");
    const install = b.addInstallFile(badge_file, "coverage.svg");
    curl.has_side_effects = true;

    const parser: *CoverageParser = .init(b, merged.output_dir, curl);
    parser.step.dependOn(&merged.runner.step);
    curl.step.dependOn(&parser.step);
    coverage.dependOn(&curl.step);
    coverage.dependOn(&install.step);
}

const CoverageParser = struct {
    const CoverageInfo = struct {
        percent_covered: []const u8,
    };
    const ParsedCovInfo = std.json.Parsed(CoverageInfo);

    step: std.Build.Step,
    report: std.Build.LazyPath,
    curl: *std.Build.Step.Run,

    pub fn init(
        b: *std.Build,
        report: std.Build.LazyPath,
        curl: *std.Build.Step.Run,
    ) *CoverageParser {
        const self = b.allocator.create(CoverageParser) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "coverage-parse",
                .owner = b,
                .makeFn = coverageParse,
            }),
            .report = report,
            .curl = curl,
        };
        return self;
    }

    fn coverageParse(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *CoverageParser = @fieldParentPtr("step", step);

        const b = step.owner;
        const allocator = b.allocator;

        const json_path = try self.report.path(b, "kcov-merged/coverage.json").getPath4(b, step);
        const contents = try b.build_root.handle.readFileAlloc(
            b.graph.io,
            json_path.sub_path,
            allocator,
            .unlimited,
        );

        const parsed: ParsedCovInfo = try std.json.parseFromSlice(
            CoverageInfo,
            allocator,
            contents,
            .{ .ignore_unknown_fields = true },
        );

        const precise_percentage = parsed.value.percent_covered;
        const last_dot = std.mem.lastIndexOfScalar(u8, precise_percentage, '.');
        const percentage = if (last_dot) |dot| precise_percentage[0..dot] else precise_percentage;
        self.curl.addArg("-s");
        self.curl.addArg(b.fmt("https://img.shields.io/badge/Coverage-{s}%25-pink", .{percentage}));
        std.log.info("Test Coverage: {s}%", .{percentage});
    }
};

const SiteBuilder = struct {
    const GoDependency = struct {
        dep: *std.Build.Dependency,
        artifact_path: std.Build.LazyPath = undefined,
        builder: *std.Build.Step.Run = undefined,
        install: *std.Build.Step.InstallFile = undefined,
    };

    const output_path = "site/";
    const output_dev_path = output_path ++ "dev/";
    const go_work = "go.work";
    const webserver_exe = "webserver";
    const devserver_exe = "devserver";
    const templ_exe = "templ";
    const air_exe = "air";

    b: *std.Build,
    optimize: std.builtin.OptimizeMode,
    site_path: std.Build.LazyPath,

    go_exe_path: []const u8,
    go_fmt_path: ?[]const u8,
    templ: GoDependency,
    air: GoDependency,
    main_builder: *std.Build.Step.Run = undefined,
    rebuild: *std.Build.Step.Compile = undefined,
    work_update_src: *std.Build.Step.UpdateSourceFiles = undefined,

    fn init(b: *std.Build, optimize: std.builtin.OptimizeMode) !?*SiteBuilder {
        const go_path = b.findProgram(&.{"go"}, &.{}) catch return null;
        const templ_dep = b.lazyDependency("templ", .{});
        const air_dep = b.lazyDependency("air", .{});
        if (templ_dep == null or air_dep == null) return null;

        const self = try b.allocator.create(SiteBuilder);
        self.* = .{
            .b = b,
            .optimize = optimize,
            .site_path = b.path(ProjectPaths.site),
            .go_exe_path = go_path,
            .go_fmt_path = b.findProgram(&.{"gofmt"}, &.{}) catch null,
            .templ = .{ .dep = templ_dep.? },
            .air = .{ .dep = air_dep.? },
        };
        return self;
    }

    /// This is fragile but I don't care
    fn build(self: *SiteBuilder) !void {
        try self.buildTempl();
        self.buildAir();
        self.buildRebuild();
        self.buildOneShots();
        self.buildWatch();
    }

    fn buildTempl(self: *SiteBuilder) !void {
        const builder = self.addRunGoBuild(.ReleaseFast);
        builder.setCwd(self.templ.dep.path("cmd/templ"));
        self.templ.artifact_path = builder.addOutputFileArg(templ_exe);
        self.templ.install = self.addInstallFile(self.templ.artifact_path, templ_exe, output_dev_path);
        self.work_update_src = try self.addGoWork();
        self.templ.builder = builder;
    }

    fn buildAir(self: *SiteBuilder) void {
        const builder = self.addRunGoBuild(.ReleaseFast);
        builder.setCwd(self.air.dep.path("."));
        self.air.artifact_path = builder.addOutputFileArg(air_exe);
        self.air.install = self.addInstallFile(self.air.artifact_path, air_exe, output_dev_path);
        self.air.builder = builder;
    }

    fn buildRebuild(self: *SiteBuilder) void {
        const b = self.b;
        self.rebuild = self.b.addExecutable(.{
            .name = "rebuild",
            .root_module = b.addModule("rebuild", .{
                .root_source_file = b.path("site/rebuild.zig"),
                .target = b.graph.host,
            }),
        });
        self.rebuild.step.dependOn(&self.templ.install.step);
    }

    fn buildOneShots(self: *SiteBuilder) void {
        const b = self.b;
        const generator = self.addRunTempl();
        generator.setCwd(self.site_path);
        generator.addArg("generate");
        generator.step.dependOn(&self.work_update_src.step);
        generator.has_side_effects = true;

        const builder = self.addRunGoBuild(self.optimize);
        builder.setCwd(self.site_path.path(b, "cmd/server"));
        builder.step.dependOn(&generator.step);
        builder.has_side_effects = true;
        self.main_builder = builder;

        const server_path = builder.addOutputFileArg(webserver_exe);
        const server_install = self.addInstallFile(server_path, webserver_exe, output_path);
        const build_site = b.step("site", "Build and install the project's website");
        build_site.dependOn(&server_install.step);

        const run_site_run: *std.Build.Step.Run = .create(b, "run site");
        run_site_run.addFileArg(server_path);
        const run_site = b.step("run-site", "Build, install, and run the project's website");
        run_site.dependOn(&run_site_run.step);
    }

    fn buildWatch(self: *SiteBuilder) void {
        const b = self.b;
        const watch_site_run: *std.Build.Step.Run = .create(b, "run air");
        watch_site_run.step.dependOn(&self.air.install.step);
        const abs_dev_path = b.pathJoin(&.{ b.install_prefix, output_dev_path });
        watch_site_run.setEnvironmentVariable("GHOTI_TEMPL_ARTIFACT", b.pathJoin(&.{
            abs_dev_path,
            self.templ.install.dest_rel_path,
        }));
        watch_site_run.setEnvironmentVariable("GHOTI_GO_PATH", self.go_exe_path);
        const output = tryAppendExe(b, b.pathJoin(&.{ abs_dev_path, devserver_exe }));
        watch_site_run.setEnvironmentVariable("GHOTI_SITE_OUTPUT", output);

        watch_site_run.addFileArg(self.air.artifact_path);
        watch_site_run.addArg("--build.cmd");
        watch_site_run.addArtifactArg(self.rebuild);
        watch_site_run.addArgs(&.{
            "--build.entrypoint",
            output,
            "--build.include_ext",
            "go,templ",
            "--build.exclude_regex",
            ".*_templ.go,.*_test.go",
            "-tmp_dir",
            abs_dev_path,
        });
        watch_site_run.setCwd(self.site_path);

        const watch_site = b.step("watch-site", "Kick off live reloading of the project's website");
        watch_site.dependOn(&watch_site_run.step);
    }

    fn addGoWork(self: *const SiteBuilder) !*std.Build.Step.UpdateSourceFiles {
        const b = self.b;
        const templ_path = try self.templ.dep.path(".").getPath4(b, null);
        const go_work_content = b.fmt(
            \\go 1.26.3
            \\
            \\use (
            \\  .
            \\  {s}
            \\)
        , .{try templ_path.toString(b.allocator)});
        const write_work = b.addWriteFile(go_work, go_work_content);
        const gen_work = write_work.getDirectory().path(b, go_work);
        const update = b.addUpdateSourceFiles();
        update.addCopyFileToSource(gen_work, ProjectPaths.site ++ go_work);
        return update;
    }

    fn addRunGoBuild(self: *const SiteBuilder, optimize: std.builtin.OptimizeMode) *std.Build.Step.Run {
        const run = self.b.addSystemCommand(&.{self.go_exe_path});
        run.addArg("build");
        run.addArgs(getGoOptimizeFlags(optimize));
        run.addArg("-o");
        return run;
    }

    fn addInstallFile(
        self: *const SiteBuilder,
        path: std.Build.LazyPath,
        executable: []const u8,
        prefix_path: []const u8,
    ) *std.Build.Step.InstallFile {
        const b = self.b;
        return b.addInstallFileWithDir(
            path,
            .{ .custom = prefix_path },
            tryAppendExe(b, executable),
        );
    }

    fn addRunTempl(self: *const SiteBuilder) *std.Build.Step.Run {
        const run: *std.Build.Step.Run = .create(self.b, "run templ");
        run.addFileArg(self.templ.artifact_path);
        return run;
    }

    fn getGoOptimizeFlags(optimize: std.builtin.OptimizeMode) []const []const u8 {
        return switch (optimize) {
            .Debug => return &.{ "-race", "-gcflags=all=-N -l" },
            .ReleaseSmall => return &.{ "-ldflags=-s -w", "-trimpath" },
            .ReleaseFast, .ReleaseSafe => return &.{"-ldflags=-s -w"},
        };
    }
};

const CollectFilesConfig = struct {
    allowed_extensions: []const []const u8 = &.{".cc"},
    dropped_files: ?[]const []const u8 = null,
    extra_files: ?[]const []const u8 = null,
    return_basenames_only: bool = false,
    dropped_extensions: ?[]const []const u8 = null,
};

fn collectFiles(
    b: *std.Build,
    directory: []const u8,
    config: CollectFilesConfig,
) ![]const []const u8 {
    const io = b.graph.io;
    var dir = try b.build_root.handle.openDir(io, directory, .{ .iterate = true });
    defer dir.close(io);

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    var paths: std.ArrayList([]const u8) = .empty;
    outer: while (try walker.next(io)) |entry| {
        if (entry.kind != .file) continue;
        for (config.allowed_extensions) |ext| {
            if (std.mem.endsWith(u8, entry.basename, ext)) break;
        } else continue;

        if (config.dropped_files) |drop| for (drop) |drop_file| {
            if (std.mem.eql(u8, drop_file, entry.basename)) continue;
        };

        if (config.dropped_extensions) |drop| for (drop) |drop_file| {
            if (std.mem.endsWith(u8, entry.basename, drop_file)) continue :outer;
        };

        if (config.return_basenames_only) {
            try paths.append(b.allocator, b.dupe(entry.basename));
        } else {
            const full_path = b.pathJoin(&.{ directory, entry.path });
            try paths.append(b.allocator, full_path);
        }
    }

    if (config.extra_files) |extra_files| {
        try paths.appendSlice(b.allocator, extra_files);
    }
    return paths.items;
}

fn tryAppendExe(b: *std.Build, raw_path: []const u8) []const u8 {
    return b.fmt("{s}{s}", .{
        raw_path,
        if (b.graph.host.result.os.tag == .windows) ".exe" else "",
    });
}
