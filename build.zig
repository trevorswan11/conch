const std = @import("std");
const builtin = @import("builtin");
const zon = @import("build.zig.zon");

const Dependency = @import("packages/third-party/Dependency.zig");
const cppcheck = @import("packages/third-party/cppcheck.zig");
const libarchive = @import("packages/third-party/libarchive.zig");
const fmt = @import("packages/third-party/fmt.zig");
const catch2 = @import("packages/third-party/catch2.zig");

const CurlBuilder = @import("packages/third-party/kcov/CurlBuilder.zig");
const BinutilsBuilder = @import("packages/third-party/kcov/BinutilsBuilder.zig");
const ElfutilsBuilder = @import("packages/third-party/kcov/ElfutilsBuilder.zig");

const LLVMBuilder = @import("packages/llvm/LLVMBuilder.zig");
const ClangBuilder = @import("packages/llvm/ClangBuilder.zig");
const LLDBuilder = @import("packages/llvm/LLDBuilder.zig");

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const llvm: *LLVMBuilder = .init(b);
    const clang: *ClangBuilder = .init(llvm);
    const cdb_gen: *CDBGenerator = .init(b);

    if (CurlBuilder.build(b, .{
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    })) |curl| {
        b.installArtifact(curl.exe);
        b.installArtifact(curl.lib);
    }

    if (BinutilsBuilder.build(b, .{
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    })) |binutils| {
        b.installArtifact(binutils.libbfd);
        b.installArtifact(binutils.libiberty);
        b.installArtifact(binutils.libopcodes);
        b.installArtifact(binutils.libsframe);
    }

    if (b.graph.host.result.os.tag == .linux) {
        if (ElfutilsBuilder.build(b, .{
            .target = b.graph.host,
            .optimize = .ReleaseFast,
        })) |elfutils| {
            b.installArtifact(elfutils.libeu);
            b.installArtifact(elfutils.libelf);
            b.installArtifact(elfutils.libdwelf);
            b.installArtifact(elfutils.libebl);
            b.installArtifact(elfutils.libdw);
        }
    }

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

    const cdb_flags = [_][]const u8{
        "-gen-cdb-fragment-path",
        getCacheRelativePath(b, &.{CDBGenerator.cdb_frags_dirname}),
    };

    try compiler_flags.appendSlice(b.allocator, &cdb_flags);
    switch (optimize) {
        .Debug => try compiler_flags.appendSlice(b.allocator, &.{ "-g", "-DDEBUG" }),
        .ReleaseSafe => try compiler_flags.appendSlice(b.allocator, &.{"-DRELEASE"}),
        .ReleaseFast, .ReleaseSmall => try compiler_flags.appendSlice(b.allocator, dist_flags),
    }

    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;
    const artifacts = try addArtifacts(b, .{
        .optimize = optimize,
        .llvm = llvm,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    clang.build();
    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .clang = clang,
        .cli = artifacts.cli,
        .cppcheck = artifacts.cppcheck.?,
    });

    try addPackageStep(b, .{
        .llvm = llvm,
        .cxx_flags = package_flags.items,
    });
}

const ProjectPaths = struct {
    const Project = struct {
        inc: []const u8,
        src: []const u8,
        tests: []const u8,

        pub fn files(self: *const Project, b: *std.Build) ![][]const u8 {
            return std.mem.concat(b.allocator, []const u8, &.{
                try collectFiles(b, self.inc, .{ .allowed_extensions = &.{".hpp"} }),
                try collectFiles(b, self.src, .{ .allowed_extensions = &.{".cpp"} }),
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
    const llvm = "packages/llvm/";
    const third_party = "packages/third-party/";

    const compressor = "apps/compressor/";
};

const ExecutableBehavior = union(enum) {
    runnable: struct {
        cmd_name: []const u8,
        cmd_desc: []const u8,
    },
    standalone: void,
};

const TestArtifacts = struct {
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
            b.installArtifact(self.runner_tests);
            b.installArtifact(self.core_tests);
            b.installArtifact(self.compiler_tests);
            b.installArtifact(self.cli_tests);
        }

        if (cdb_steps) |cdb| {
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
    target: ?std.Build.ResolvedTarget = null,
    optimize: std.builtin.OptimizeMode,
    llvm: *LLVMBuilder,
    cxx_flags: []const []const u8,
    cdb_steps: ?*std.ArrayList(*std.Build.Step),
    behavior: ?ExecutableBehavior = null,
    auto_install: bool = true,
    packaging: bool = false,
}) !struct {
    libcore: *std.Build.Step.Compile,
    libcompiler: *std.Build.Step.Compile,
    libcli: *std.Build.Step.Compile,
    cli: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
} {
    const target = config.target orelse b.graph.host;
    const building_for_host = config.target == null;

    const magic_enum = b.dependency("magic_enum", .{});
    const magic_enum_inc = magic_enum.path("include");

    const fmt_dep = fmt.build(b, .{
        .target = target,
        .optimize = config.optimize,
    });

    // Shared core functionality
    const libcore = createLibrary(b, .{
        .name = "core",
        .target = target,
        .optimize = config.optimize,
        .include_paths = &.{b.path(ProjectPaths.core.inc)},
        .system_include_paths = &.{magic_enum_inc},
        .cxx = .{
            .files = try collectFiles(b, ProjectPaths.core.src, .{}),
            .flags = config.cxx_flags,
        },
        .link_libraries = &.{fmt_dep.artifact},
    });
    if (config.auto_install) b.installArtifact(libcore);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcore.step);

    // LLVM is compiled from source because I like burning compute or something
    config.llvm.build(.{
        .target = target,
        .behavior = if (config.packaging)
            .package
        else
            .{ .allow_kaleidoscope = config.auto_install },
    });

    // The actual compiler static library
    const libcompiler = createLibrary(b, .{
        .name = "compiler",
        .target = target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
        },
        .system_include_paths = &.{magic_enum_inc},
        .link_libraries = &.{ libcore, fmt_dep.artifact },
        .cxx = .{
            .files = try collectFiles(b, ProjectPaths.compiler.src, .{}),
            .flags = config.cxx_flags,
        },
    });
    if (config.auto_install) b.installArtifact(libcompiler);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcompiler.step);

    // The CLI library is stripped of main
    const libcli = createLibrary(b, .{
        .name = "cli",
        .target = target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.cli.inc),
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
        },
        .system_include_paths = &.{magic_enum_inc},
        .link_libraries = &.{ libcompiler, fmt_dep.artifact },
        .cxx = .{
            .files = try collectFiles(b, ProjectPaths.cli.src, .{
                .dropped_files = &.{"main.cpp"},
            }),
            .flags = config.cxx_flags,
        },
    });
    if (config.auto_install) b.installArtifact(libcli);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcli.step);

    // The shippable executable links only against libcli which has a transitive dep of the compiler
    const cli = createExecutable(b, .{
        .name = "conch",
        .target = target,
        .optimize = config.optimize,
        .include_paths = &.{b.path(ProjectPaths.cli.inc)},
        .cxx = .{
            .files = &.{ProjectPaths.cli.src ++ "main.cpp"},
            .flags = config.cxx_flags,
        },
        .link_libraries = &.{ libcli, fmt_dep.artifact },
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
    if (building_for_host) {
        const test_runner = b.path(ProjectPaths.test_runner ++ "main.zig");
        const catch2_dep = catch2.build(b, .{
            .target = target,
            .optimize = config.optimize,
        });

        // The runner has standalone tests
        const runner_tests = b.addTest(.{
            .name = "runner_tests",
            .root_module = b.createModule(.{
                .root_source_file = test_runner,
                .optimize = config.optimize,
                .target = target,
                .link_libc = true,
            }),
        });

        const runner_cmd = b.addRunArtifact(runner_tests);
        const runner_step = b.step("test-runner", "Run test instrumentation tests");
        runner_step.dependOn(&runner_cmd.step);

        // Core tests depend on the test runner but not LLVM
        const core_tests = createExecutable(b, .{
            .name = "core_tests",
            .zig_main = test_runner,
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.core.tests),
            },
            .system_include_paths = &.{magic_enum_inc},
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.core.tests, .{
                    .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
                }),
                .flags = config.cxx_flags,
            },
            .link_libraries = &.{ libcore, catch2_dep.artifact, fmt_dep.artifact },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-core",
                    .cmd_desc = "Run core unit tests",
                },
            },
        });

        // Compiler tests can pull in core helpers
        const compiler_tests = createExecutable(b, .{
            .name = "compiler_tests",
            .zig_main = test_runner,
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.compiler.tests),
            },
            .system_include_paths = &.{magic_enum_inc},
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.compiler.tests, .{
                    .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
                }),
                .flags = config.cxx_flags,
            },
            .link_libraries = &.{ libcompiler, catch2_dep.artifact, fmt_dep.artifact },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-compiler",
                    .cmd_desc = "Run compiler unit tests",
                },
            },
        });

        // CLI tests can pull in core helpers
        const cli_tests = createExecutable(b, .{
            .name = "cli_tests",
            .zig_main = b.path(ProjectPaths.test_runner ++ "main.zig"),
            .target = target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.cli.inc),
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.cli.tests),
            },
            .system_include_paths = &.{magic_enum_inc},
            .cxx = .{
                .files = try collectFiles(b, ProjectPaths.cli.tests, .{
                    .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
                }),
                .flags = config.cxx_flags,
            },
            .link_libraries = &.{ libcompiler, libcli, catch2_dep.artifact, fmt_dep.artifact },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-cli",
                    .cmd_desc = "Run CLI unit tests",
                },
            },
        });

        tests = .{
            .runner_tests = runner_tests,
            .core_tests = core_tests,
            .compiler_tests = compiler_tests,
            .cli_tests = cli_tests,
        };
        try tests.?.configure(b, config.auto_install, config.cdb_steps);
    }

    const cppcheck_dep: ?Dependency = if (building_for_host) try cppcheck.build(b, .{
        .target = target,
        .optimize = .ReleaseFast,
    }) else null;

    return .{
        .libcore = libcore,
        .libcompiler = libcompiler,
        .libcli = libcli,
        .cli = cli,
        .tests = tests,
        .cppcheck = if (cppcheck_dep) |dep| dep.artifact else null,
    };
}

const SystemLibraries = struct {
    search_paths: []const std.Build.LazyPath,
    libs: []const []const u8,
};

const CXXOpts = struct {
    files: []const []const u8,
    flags: []const []const u8,
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
    cxx: CXXOpts,
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
        .files = config.cxx.files,
        .flags = config.cxx.flags,
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
    include_paths: ?[]const std.Build.LazyPath = null,
    system_include_paths: ?[]const std.Build.LazyPath = null,
    source_root: ?std.Build.LazyPath = null,
    cxx: ?CXXOpts = null,
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

    if (config.include_paths) |include_paths| {
        for (include_paths) |include| {
            mod.addIncludePath(include);
        }
    }

    if (config.system_include_paths) |system_includes| {
        for (system_includes) |include| {
            mod.addSystemIncludePath(include);
        }
    }

    for (config.link_libraries) |library| {
        mod.linkLibrary(library);
    }

    if (config.cxx) |cxx| {
        mod.addCSourceFiles(.{
            .root = config.source_root,
            .files = cxx.files,
            .flags = cxx.flags,
            .language = .cpp,
        });
    }

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
    cdb_gen: *CDBGenerator,
    clang: *ClangBuilder,
    cli: *std.Build.Step.Compile,
    cppcheck: *std.Build.Step.Compile,
}) !void {
    const tooling_sources = try collectToolingFiles(b);

    const cdb_step = b.step("cdb", "Generate " ++ CDBGenerator.cdb_filename);
    cdb_step.dependOn(&config.cdb_gen.step);
    b.getInstallStep().dependOn(&config.cdb_gen.step);

    try addFmtStep(b, .{
        .tooling_sources = tooling_sources,
        .clang = config.clang,
    });

    const check_step = try addStaticAnalysisStep(b, .{
        .tooling_sources = tooling_sources,
        .cppcheck = config.cppcheck,
        .cdb_gen = config.cdb_gen,
    });
    check_step.dependOn(&config.cdb_gen.step);

    const cloc: *LOCCounter = .init(b);
    const cloc_step = b.step("cloc", "Count lines of code across the project");
    cloc_step.dependOn(&cloc.step);
}

fn addFmtStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    clang: *ClangBuilder,
}) !void {
    const clang_format = config.clang.clang_tools.clang_format;
    const zig_paths = try collectFiles(b, "packages", .{
        .allowed_extensions = &.{".zig"},
        .extra_files = &.{
            "build.zig",
            "build.zig.zon",
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
}

fn addStaticAnalysisStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    cppcheck: *std.Build.Step.Compile,
    cdb_gen: *CDBGenerator,
}) !*std.Build.Step {
    const check_step = b.step("check", "Run static analysis on all project files");
    const cppcheck_run = b.addRunArtifact(config.cppcheck);

    const installed_cppcheck_cache_path = getCacheRelativePath(b, &.{"cppcheck"});
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
    });

    // Other spurious warnings
    const suppressions: []const []const u8 = &.{
        "checkersReport",
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
    };

    inline for (suppressions) |suppression| {
        cppcheck_run.addArg("--suppress=" ++ suppression);
    }

    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.core.tests));
    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.compiler.tests));
    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.cli.tests));
    cppcheck_run.addPrefixedDirectoryArg("-i", b.path(ProjectPaths.llvm));

    const cppcheck_cache_install = b.addInstallDirectory(.{
        .source_dir = cppcheck_cache,
        .install_dir = .{ .custom = ".." },
        .install_subdir = installed_cppcheck_cache_path,
    });

    cppcheck_cache_install.step.dependOn(&config.cppcheck.step);
    check_step.dependOn(&cppcheck_cache_install.step);
    check_step.dependOn(&cppcheck_run.step);
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

        const extensions = [_][]const u8{ ".cpp", ".hpp", ".zig", ".conch" };
        var files: std.ArrayList([]const u8) = .empty;

        const dropped_list = try collectFiles(b, ProjectPaths.llvm ++ "sources", .{
            .allowed_extensions = &.{".zig"},
            .return_basenames_only = true,
            .extra_files = try collectFiles(b, ProjectPaths.third_party, .{
                .allowed_extensions = &.{".zig"},
                .return_basenames_only = true,
            }),
        });

        try files.appendSlice(
            b.allocator,
            try collectFiles(b, "packages", .{
                .allowed_extensions = &extensions,
                .extra_files = &.{"build.zig"},
                .dropped_files = dropped_list,
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
    llvm: *LLVMBuilder,
    cxx_flags: []const []const u8,
}) !void {
    const libarchive_dep = libarchive.build(b);
    const compressor = createExecutable(b, .{
        .name = "compressor",
        .zig_main = b.path(ProjectPaths.compressor ++ "main.zig"),
        .target = b.graph.host,
        .optimize = .ReleaseFast,
        .link_libraries = &.{libarchive_dep.artifact},
        .behavior = .standalone,
    });

    // Always clean up the compressed dir before packaging
    const package_parent_dirname = "package";
    const cleaner = b.addRemoveDirTree(.{
        .cwd_relative = b.pathJoin(&.{ b.install_prefix, package_parent_dirname }),
    });
    compressor.step.dependOn(&cleaner.step);

    const package_step = b.step("package", "Package artifacts for a new release");
    package_step.dependOn(&compressor.step);
    const version = zon.version;

    const ArchiveBehavior = struct {
        compressor_arg: enum { zip, zst },
        file_extension: []const u8,
        artifact_dirname: []const u8,
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

        artifacts.cli.out_filename = blk: {
            const name = artifacts.cli.name;
            break :blk if (target.result.os.tag == .windows)
                b.fmt("{s}-{s}.exe", .{ name, version })
            else
                b.fmt("{s}-{s}", .{ name, version });
        };
        artifacts.cli.root_module.strip = true;

        const package_artifact_dirname = b.fmt("conch-{s}-{s}", .{
            try query.zigTriple(b.allocator),
            version,
        });

        const staging = b.addWriteFiles();
        const artifact_dest_path = b.fmt("{s}/{s}", .{ package_artifact_dirname, artifacts.cli.out_filename });
        _ = staging.addCopyFile(artifacts.cli.getEmittedBin(), artifact_dest_path);

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
                .artifact_dirname = package_artifact_dirname,
                .skip = target.result.os.tag != .windows,
            },
            .{
                .compressor_arg = .zst,
                .file_extension = "tar.zst",
                .artifact_dirname = package_artifact_dirname,
            },
        };

        for (archives) |archive| {
            if (archive.skip) continue;
            const out_name = b.fmt("{s}.{s}", .{
                archive.artifact_dirname,
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

fn collectFiles(
    b: *std.Build,
    directory: []const u8,
    config: struct {
        allowed_extensions: []const []const u8 = &.{".cpp"},
        dropped_files: ?[]const []const u8 = null,
        extra_files: ?[]const []const u8 = null,
        return_basenames_only: bool = false,
    },
) ![]const []const u8 {
    var dir = try b.build_root.handle.openDir(directory, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    var paths: std.ArrayList([]const u8) = .empty;
    collector: while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        for (config.allowed_extensions) |ext| {
            if (std.mem.endsWith(u8, entry.basename, ext)) break;
        } else continue :collector;

        if (config.dropped_files) |drop| for (drop) |drop_file| {
            if (std.mem.eql(u8, drop_file, entry.basename)) continue :collector;
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
