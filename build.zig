const std = @import("std");
const builtin = @import("builtin");
const zon = @import("build.zig.zon");

pub const stdx = @import("stdx");
pub const zlib = stdx.zlib;
pub const zstd = stdx.zstd;
const replxx = @import("third-party/replxx.zig");

const CDBGenerator = stdx.CDBGenerator;
const LOCCounter = stdx.LOCCounter;
const RemoveDir = stdx.RemoveDir;

const LLVMBuilder = @import("third-party/llvm/LLVMBuilder.zig");
const ClangBuilder = @import("third-party/llvm/ClangBuilder.zig");
const LLDBuilder = @import("third-party/llvm/LLDBuilder.zig");
const SiteBuilder = @import("third-party/go/SiteBuilder.zig");

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const profile = b.option(bool, "profile", "Enable chromium tracing") orelse false;
    const stdx_dep = b.dependency("stdx", .{
        .target = b.graph.host,
        .optimize = optimize,
        .profile = profile,
        .building_for_dep = true,
        .run_cdb_gen = false,
    });

    const llvm: *LLVMBuilder = .init(b);
    const clang: *ClangBuilder = .init(llvm);
    const cdb_gen: *CDBGenerator = .init(b);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &stdx.utils.base_cxx_flags);
    try compiler_flags.appendSlice(b.allocator, &.{ "-DMAGIC_ENUM_RANGE_MAX=255", "-DREPLXX_STATIC" });
    const dist_flags: []const []const u8 = &.{ "-DNDEBUG", "-DGHOTI_DIST" };

    var package_flags = try compiler_flags.clone(b.allocator);
    try package_flags.appendSlice(b.allocator, dist_flags);

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
        .stdx_dep = stdx_dep,
        .profile = profile,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    clang.build();
    const cppcheck = stdx_dep.artifact("cppcheck");
    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .clang = clang,
        .cppcheck = cppcheck,
        .site_builder = site_builder,
    });

    try addPackageStep(b, .{
        .llvm = llvm,
        .cxx_flags = package_flags.items,
        .compressor = stdx_dep.artifact("compressor"),
    });

    if (stdx.KcovBuilder.allowedTarget(b.graph.host)) {
        if (artifacts.tests) |tests| try stdx.steps.addCoverage(b, .{
            .curl = stdx_dep.artifact("curl"),
            .kcov = stdx_dep.artifact("kcov"),
            .run_configs = &.{
                .{
                    .artifact = tests.support_tests,
                    .include_patterns = &.{ ProjectPaths.support.src, ProjectPaths.support.inc },
                },
                .{
                    .artifact = tests.compiler_tests,
                    .include_patterns = &.{ ProjectPaths.compiler.src, ProjectPaths.compiler.inc },
                },
                .{
                    .artifact = tests.driver_tests,
                    .include_patterns = &.{ ProjectPaths.driver.src, ProjectPaths.driver.inc },
                },
            },
        });
    }
}

pub const ProjectPaths = struct {
    const Project = struct {
        inc: []const u8,
        src: []const u8,
        tests: []const u8,

        pub fn files(self: *const Project, b: *std.Build) ![][]const u8 {
            return std.mem.concat(b.allocator, []const u8, &.{
                try stdx.utils.collectFiles(b, self.inc, .{ .allowed_extensions = &.{".hh"} }),
                try stdx.utils.collectFiles(b, self.src, .{ .allowed_extensions = &.{".cc"} }),
                try stdx.utils.collectFiles(b, self.tests, .{ .allowed_extensions = &.{ ".hh", ".cc" } }),
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

    pub fn collectToolingPaths(b: *std.Build) !stdx.steps.FmtPaths {
        const cxx_paths = try std.mem.concat(b.allocator, []const u8, &.{
            try compiler.files(b),
            try driver.files(b),
            try support.files(b),
        });

        return .{ .zig = &.{
            "build.zig",
            "build.zig.zon",
            site ++ "rebuild.zig",
            third_party ++ "go/SiteBuilder.zig",
        }, .cxx = cxx_paths };
    }

    const stdlib = "lib/std/";
    pub const site = "site/";
    const third_party = "third-party/";
};

const TestArtifacts = struct {
    const WebserverTests = struct {
        run: *std.Build.Step.Run,
        install: *std.Build.Step.InstallDir,
    };

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
            self.support_tests,
            self.compiler_tests,
            self.driver_tests,
        };

        const test_step = b.step("test", "Run all unit tests");
        for (artifacts) |artifact| {
            _ = stdx.utils.ExecutableBehavior.installArtifact(
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

fn addArtifacts(b: *std.Build, config: struct {
    target: ?std.Build.ResolvedTarget = null,
    optimize: std.builtin.OptimizeMode,
    llvm: *LLVMBuilder,
    cxx_flags: []const []const u8,
    cdb_steps: ?*std.ArrayList(*std.Build.Step),
    behavior: ?stdx.utils.ExecutableBehavior = null,
    auto_install: bool = true,
    packaging: bool = false,
    install_tests_only: bool = true,
    site_builder: ?*SiteBuilder = null,
    stdx_dep: *std.Build.Dependency,
    profile: bool = false,
}) !struct {
    libsupport: *std.Build.Step.Compile,
    libcompiler: *std.Build.Step.Compile,
    libdriver: *std.Build.Step.Compile,
    ghoti: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
} {
    const target = config.target orelse b.graph.host;
    const config_h = b.addConfigHeader(.{ .include_path = "ghoti/config.h" }, .{
        .GHOTI_VERSION_STR = version_str,
        .GHOTI_VERSION_MAJOR = @as(i64, version.major),
        .GHOTI_VERSION_MINOR = @as(i64, version.minor),
        .GHOTI_VERSION_PATCH = @as(i64, version.patch),
        .GHOTI_VERSION_PRE = version.pre orelse "",
        .GHOTI_GIT_INFO = stdx.utils.getGitInfo(b),
        .GHOTI_WINDOWS = target.result.os.tag == .windows,
        .GHOTI_LINUX = target.result.os.tag == .linux,
        .GHOTI_APPLE = target.result.os.tag == .macos,
    });
    const libstdx = config.stdx_dep.artifact("stdx");
    const building_for_host = config.target == null;

    const cli11 = b.dependency("cli11", .{});
    const cli11_inc = cli11.path("include");

    const replxx_dep = replxx.build(b, .{
        .target = target,
        .optimize = config.optimize,
    });

    // Shared core functionality
    const libsupport = b.addLibrary(.{
        .name = "support",
        .root_module = stdx.utils.createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{b.path(ProjectPaths.support.inc)},
            .cxx = .{
                .files = try stdx.utils.collectFiles(b, ProjectPaths.support.src, .{}),
                .flags = config.cxx_flags,
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{ replxx_dep.artifact, libstdx },
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
        .root_module = stdx.utils.createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
            },
            .config_headers = &.{config_h},
            .link_libraries = &.{ libsupport, libstdx },
            .cxx = .{
                .files = try stdx.utils.collectFiles(b, ProjectPaths.compiler.src, .{}),
                .flags = config.cxx_flags,
            },
        }),
    });
    if (config.auto_install) b.installArtifact(libcompiler);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcompiler.step);

    // The user-facing library
    const libdriver = b.addLibrary(.{
        .name = "driver",
        .root_module = stdx.utils.createModule(b, .{
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.driver.inc),
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
            },
            .system_include_paths = &.{cli11_inc},
            .config_headers = &.{config_h},
            .link_libraries = &.{ libcompiler, libstdx, replxx_dep.artifact },
            .cxx = .{
                .files = try stdx.utils.collectFiles(b, ProjectPaths.driver.src, .{}),
                .flags = config.cxx_flags,
            },
        }),
    });
    if (config.auto_install) b.installArtifact(libdriver);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libdriver.step);

    // The shippable executable
    const ghoti = stdx.utils.createExecutable(b, .{
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
        .system_include_paths = &.{cli11_inc},
        .link_libraries = &.{ libdriver, libstdx, replxx_dep.artifact },
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
        const libcatch2 = config.stdx_dep.artifact("catch2");

        // Support's tests depend on the test runner but not LLVM
        const support_tests = stdx.builders.strappedTest(config.stdx_dep.builder, .{
            .target = target,
            .optimize = config.optimize,
            .libstdx = libstdx,
            .libcatch2 = libcatch2,
            .cxx_files = try stdx.utils.collectFiles(b, ProjectPaths.support.tests, .{}),
            .cxx_flags = config.cxx_flags,
            .profile = config.profile,
            .include_paths = &.{
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.support.tests),
            },
            .link_libraries = &.{libsupport},
            .config_headers = &.{config_h},
            .system_include_paths = &.{cli11_inc},
            .executable_config = .{
                .name = "support",
                .behavior = config.behavior orelse .{
                    .installable = .{
                        .cmd_name = "test-support",
                        .cmd_desc = "Build/run support's unit tests",
                        .install_dir = test_install_dir,
                        .install_only = config.install_tests_only,
                    },
                },
            },
            .asking_builder = b,
        });

        const compiler_tests = stdx.builders.strappedTest(config.stdx_dep.builder, .{
            .target = target,
            .optimize = config.optimize,
            .libstdx = libstdx,
            .libcatch2 = libcatch2,
            .cxx_files = try stdx.utils.collectFiles(b, ProjectPaths.compiler.tests, .{}),
            .cxx_flags = config.cxx_flags,
            .profile = config.profile,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.compiler.tests),
            },
            .link_libraries = &.{libcompiler},
            .config_headers = &.{config_h},
            .system_include_paths = &.{cli11_inc},
            .executable_config = .{
                .name = "compiler",
                .behavior = config.behavior orelse .{
                    .installable = .{
                        .cmd_name = "test-compiler",
                        .cmd_desc = "Build/run compiler unit tests",
                        .install_dir = test_install_dir,
                        .install_only = config.install_tests_only,
                    },
                },
            },
            .asking_builder = b,
        });

        const driver_tests = stdx.builders.strappedTest(config.stdx_dep.builder, .{
            .target = target,
            .optimize = config.optimize,
            .libstdx = libstdx,
            .libcatch2 = libcatch2,
            .cxx_files = try stdx.utils.collectFiles(b, ProjectPaths.driver.tests, .{}),
            .cxx_flags = config.cxx_flags,
            .profile = config.profile,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.driver.inc),
                b.path(ProjectPaths.support.inc),
                b.path(ProjectPaths.driver.tests),
            },
            .link_libraries = &.{
                libcompiler,
                libdriver,
                replxx_dep.artifact,
            },
            .config_headers = &.{config_h},
            .system_include_paths = &.{cli11_inc},
            .executable_config = .{
                .name = "driver",
                .behavior = config.behavior orelse .{
                    .installable = .{
                        .cmd_name = "test-driver",
                        .cmd_desc = "Build/run the driver's unit tests",
                        .install_dir = test_install_dir,
                        .install_only = config.install_tests_only,
                    },
                },
            },
            .asking_builder = b,
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
            .support_tests = support_tests,
            .compiler_tests = compiler_tests,
            .driver_tests = driver_tests,
            .webserver_tests = webserver_tests,
        };
        try tests.?.configure(b, config.cdb_steps, test_install_dir, config.install_tests_only);
    }

    return .{
        .libsupport = libsupport,
        .libcompiler = libcompiler,
        .libdriver = libdriver,
        .ghoti = ghoti,
        .tests = tests,
    };
}

const counted_extensions = [_][]const u8{
    ".cc", ".hh",    ".inc",  ".zig", ".gh",
    ".go", ".templ", ".html", ".css",
};

fn addTooling(b: *std.Build, config: struct {
    cdb_gen: *CDBGenerator,
    clang: *ClangBuilder,
    cppcheck: *std.Build.Step.Compile,
    site_builder: ?*SiteBuilder,
}) !void {
    const cdb_step = b.step("cdb", "Generate " ++ CDBGenerator.cdb_filename);
    cdb_step.dependOn(&config.cdb_gen.step);
    b.getInstallStep().dependOn(&config.cdb_gen.step);

    const fmt_steps = try stdx.steps.addFmt(b, .{
        .paths = try ProjectPaths.collectToolingPaths(b),
        .formatter = .{ .artifact = config.clang.clang_tools.clang_format },
    });

    if (config.site_builder) |site| if (site.go_fmt_path) |go_fmt| {
        const go_paths = try stdx.utils.collectFiles(b, ProjectPaths.site, .{
            .allowed_extensions = &.{".go"},
            .dropped_extensions = &.{"_templ.go"},
        });

        const go_formatter = b.addSystemCommand(&.{ go_fmt, "-w" });
        go_formatter.addArgs(go_paths);
        fmt_steps.fmt.dependOn(&go_formatter.step);

        const templ_formatter = site.addRunTempl();
        templ_formatter.setCwd(site.site_path);
        templ_formatter.addArgs(&.{ "fmt", "." });
        fmt_steps.fmt.dependOn(&templ_formatter.step);

        const go_formatter_check = b.addSystemCommand(&.{ go_fmt, "-d" });
        go_formatter_check.addArgs(go_paths);
        fmt_steps.fmt_check.dependOn(&go_formatter_check.step);

        const templ_formatter_check = site.addRunTempl();
        templ_formatter_check.setCwd(site.site_path);
        templ_formatter_check.addArgs(&.{ "fmt", "-fail", "." });
        fmt_steps.fmt_check.dependOn(&templ_formatter_check.step);
    };

    const check_step = stdx.steps.addCppcheck(b, .{
        .cppcheck = config.cppcheck,
        .cdb_gen = config.cdb_gen,
        .extra_suppress_patterns = &.{
            "--suppress=*:*llvm/*",
            "--suppress=*:*CLI/*",
        },
    });

    if (config.site_builder) |site| {
        const go_vet = b.addSystemCommand(&.{ site.go_exe_path, "vet", "./..." });
        go_vet.setCwd(site.site_path);
        go_vet.step.dependOn(&site.main_builder.step);
        check_step.dependOn(&go_vet.step);
    }
    check_step.dependOn(&config.cdb_gen.step);

    const counted_files = try std.mem.concat(b.allocator, []const u8, &.{
        try stdx.utils.collectFiles(b, "lib", .{
            .allowed_extensions = &counted_extensions,
            .extra_files = &.{"build.zig"},
        }),
        try stdx.utils.collectFiles(b, "ghoti", .{ .allowed_extensions = &counted_extensions }),
        try stdx.utils.collectFiles(b, "site", .{ .allowed_extensions = &counted_extensions }),
    });

    const cloc: *LOCCounter = .init(b, counted_files);
    const cloc_step = b.step("cloc", "Count lines of code across the project");
    cloc_step.dependOn(&cloc.step);
}

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
    compressor: *std.Build.Step.Compile,
}) !void {
    const packager: *stdx.Packager = .init(b, .{
        .compressor = config.compressor,
    });

    for (target_queries) |query| {
        const target = b.resolveTargetQuery(query);
        const stdx_dep = b.dependency("stdx", .{
            .target = target,
            .optimize = .ReleaseFast,
            .building_for_dep = true,
            .run_cdb_gen = false,
            .packaging = true,
        });

        const artifacts = try addArtifacts(b, .{
            .target = target,
            .optimize = .ReleaseFast,
            .llvm = config.llvm.clone(),
            .cxx_flags = config.cxx_flags,
            .cdb_steps = null,
            .behavior = .standalone,
            .auto_install = false,
            .packaging = true,
            .stdx_dep = stdx_dep,
        });

        artifacts.ghoti.out_filename = stdx.utils.tryAppendExe(
            b.allocator,
            target,
            b.fmt("{s}-{s}", .{ artifacts.ghoti.name, version_str }),
        );
        artifacts.ghoti.root_module.strip = true;

        const package_artifact_dirname = b.fmt("ghoti-{s}-{s}", .{ try query.zigTriple(b.allocator), version_str });
        const staging = b.addWriteFiles();
        const copy_paths = [_]struct { std.Build.LazyPath, []const u8 }{
            .{ artifacts.ghoti.getEmittedBin(), artifacts.ghoti.out_filename },
            .{ b.path("LICENSE"), "LICENSE" },
            .{ b.path("README.md"), "README.md" },
            .{ b.path(".github/CHANGELOG.md"), "CHANGELOG.md" },
        };

        for (copy_paths) |path| {
            const src, const dst = path;
            _ = staging.addCopyFile(src, b.fmt("{s}/{s}", .{ package_artifact_dirname, dst }));
        }

        packager.addArchives(.{
            .target = target,
            .staging = staging,
            .output_dir_basename = package_artifact_dirname,
        });
    }
}
