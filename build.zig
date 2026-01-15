const std = @import("std");
const builtin = @import("builtin");

const version = "v0.0.1";

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const cdb_gen: *CdbGenerator = .init(b);
    const flag_opts = addFlagOptions(b);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
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
        .target = target,
        .optimize = optimize,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
        .skip_tests = flag_opts.skip_tests,
        .skip_cppcheck = flag_opts.skip_cppcheck,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .conch_tests = artifacts.conch_tests,
        .cppcheck = artifacts.cppcheck,
        .clean_cache = flag_opts.clean_cache,
    });

    try addPackageStep(b, .{
        .cxx_flags = package_flags.items,
        .compile_only = flag_opts.compile_only,
        .version = version,
    });
}

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

    const clean_cache = (builtin.os.tag != .windows) and (b.option(
        bool,
        "clean-cache",
        "Clean the build cache with the clean step",
    ) orelse false);

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
        with_args: bool,
    },
    standalone: void,
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
    libconch: *std.Build.Step.Compile,
    conch: *std.Build.Step.Compile,
    libcatch2: ?*std.Build.Step.Compile,
    conch_tests: ?*std.Build.Step.Compile,
    cppcheck: ?*std.Build.Step.Compile,
} {
    const libconch = createLibrary(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_path = b.path("include"),
        .cxx_files = try collectFiles(b, "src", .{ .dropped_files = &.{"main.cpp"} }),
        .flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libconch);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libconch.step);

    const cxx_main = b.pathJoin(&.{ "src", "main.cpp" });
    const conch = createExecutable(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{b.path("include")},
        .cxx_files = &.{cxx_main},
        .cxx_flags = config.cxx_flags,
        .link_libraries = &.{libconch},
        .behavior = config.behavior orelse .{
            .runnable = .{
                .cmd_name = "run",
                .cmd_desc = "Run conch with provided command line arguments",
                .with_args = true,
            },
        },
    });
    if (config.auto_install) b.installArtifact(conch);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &conch.step);

    var libcatch2: ?*std.Build.Step.Compile = null;
    var conch_tests: ?*std.Build.Step.Compile = null;
    if (!config.skip_tests) {
        const catch2_dir = b.pathJoin(&.{ "vendor", "catch2" });
        libcatch2 = createLibrary(b, .{
            .name = "catch2",
            .target = config.target,
            .optimize = .ReleaseSafe,
            .include_path = b.path(catch2_dir),
            .cxx_files = try collectFiles(b, catch2_dir, .{}),
            .flags = config.cxx_flags,
        });
        if (config.auto_install) b.installArtifact(libcatch2.?);
        if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcatch2.?.step);

        const helper_dir = b.pathJoin(&.{ "tests", "helpers" });
        conch_tests = createExecutable(b, .{
            .name = "conch_tests",
            .zig_main = b.path(b.pathJoin(&.{ "tests", "main.zig" })),
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{ b.path("include"), b.path(catch2_dir), b.path(helper_dir) },
            .cxx_files = try collectFiles(b, "tests", .{}),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ libconch, libcatch2.? },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test",
                    .cmd_desc = "Run unit tests",
                    .with_args = false,
                },
            },
        });

        if (config.auto_install) b.installArtifact(conch_tests.?);
        if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &conch_tests.?.step);
    }

    const cppcheck = if (config.skip_cppcheck) null else try compileCppcheck(b, config.target);

    return .{
        .libconch = libconch,
        .conch = conch,
        .libcatch2 = libcatch2,
        .conch_tests = conch_tests,
        .cppcheck = cppcheck,
    };
}

/// Compiles cppcheck from source using the flags given by
/// https://github.com/danmar/cppcheck#g-for-experts
fn compileCppcheck(b: *std.Build, target: std.Build.ResolvedTarget) !*std.Build.Step.Compile {
    const cppcheck_includes: []const std.Build.LazyPath = &.{
        b.path(getCppcheckRelativePath(b, &.{"externals"})),
        b.path(getCppcheckRelativePath(b, &.{ "externals", "simplecpp" })),
        b.path(getCppcheckRelativePath(b, &.{ "externals", "tinyxml2" })),
        b.path(getCppcheckRelativePath(b, &.{ "externals", "picojson" })),
        b.path(getCppcheckRelativePath(b, &.{"lib"})),
        b.path(getCppcheckRelativePath(b, &.{"frontend"})),
    };

    var cppcheck_sources: std.ArrayList([]const u8) = .empty;
    try cppcheck_sources.appendSlice(b.allocator, &.{
        getCppcheckRelativePath(b, &.{ "externals", "simplecpp", "simplecpp.cpp" }),
        getCppcheckRelativePath(b, &.{ "externals", "tinyxml2", "tinyxml2.cpp" }),
    });

    const cppcheck_glob_srcs: []const []const []const u8 = &.{
        try collectFiles(b, getCppcheckRelativePath(b, &.{"frontend"}), .{}),
        try collectFiles(b, getCppcheckRelativePath(b, &.{"cli"}), .{}),
        try collectFiles(b, getCppcheckRelativePath(b, &.{"lib"}), .{}),
    };

    for (cppcheck_glob_srcs) |glob| {
        try cppcheck_sources.appendSlice(b.allocator, glob);
    }

    // The path needs to be fixed on windows due to cppcheck internals
    const cfg_path = blk: {
        const raw_cfg_path = getCppcheckRelativePath(b, &.{});
        if (builtin.os.tag == .windows) {
            break :blk try std.mem.replaceOwned(u8, b.allocator, raw_cfg_path, "\\", "/");
        }
        break :blk raw_cfg_path;
    };

    const files_dir_define = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{cfg_path},
    );

    return createExecutable(b, .{
        .name = "cppcheck",
        .target = target,
        .optimize = .ReleaseSafe,
        .include_paths = cppcheck_includes,
        .cxx_files = cppcheck_sources.items,
        .cxx_flags = &.{ files_dir_define, "-Uunix" },
    });
}

fn createLibrary(b: *std.Build, config: struct {
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_path: std.Build.LazyPath,
    cxx_files: []const []const u8,
    flags: []const []const u8,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });
    mod.addIncludePath(config.include_path);
    mod.addCSourceFiles(.{
        .files = config.cxx_files,
        .flags = config.flags,
        .language = .cpp,
    });

    const lib = b.addLibrary(.{
        .name = config.name,
        .root_module = mod,
    });
    return lib;
}

fn createExecutable(b: *std.Build, config: struct {
    name: []const u8,
    zig_main: ?std.Build.LazyPath = null,
    target: ?std.Build.ResolvedTarget,
    optimize: ?std.builtin.OptimizeMode,
    include_paths: []const std.Build.LazyPath,
    cxx_files: []const []const u8,
    cxx_flags: []const []const u8,
    link_libraries: []const *std.Build.Step.Compile = &.{},
    behavior: ExecutableBehavior = .standalone,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .root_source_file = config.zig_main,
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });

    for (config.include_paths) |include| {
        mod.addIncludePath(include);
    }

    for (config.link_libraries) |library| {
        mod.linkLibrary(library);
    }

    mod.addCSourceFiles(.{
        .files = config.cxx_files,
        .flags = config.cxx_flags,
        .language = .cpp,
    });

    const exe = b.addExecutable(.{
        .name = config.name,
        .root_module = mod,
    });

    switch (config.behavior) {
        .runnable => |run| {
            const run_cmd = b.addRunArtifact(exe);
            run_cmd.step.dependOn(b.getInstallStep());

            if (run.with_args) if (b.args) |args| {
                run_cmd.addArgs(args);
            };

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

        // Hashed updates are generated by the compiler, so grab the most recent for the cdb
        while (try dir_iter.next()) |entry| {
            if (entry.kind != .file) continue;
            const stat = try dir.statFile(entry.name);
            const first_dot = std.mem.indexOf(u8, entry.name, ".") orelse return error.InvalidSourceFile;
            const base_name = entry.name[0 .. first_dot + 4];

            const gop = try newest_frags.getOrPut(try allocator.dupe(u8, base_name));
            if (!gop.found_existing or stat.mtime > gop.value_ptr.mtime) {
                if (gop.found_existing) allocator.free(gop.value_ptr.name);
                gop.value_ptr.* = .{
                    .name = try allocator.dupe(u8, entry.name),
                    .mtime = stat.mtime,
                };
            }
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
            const contents = try cache_root.readFileAlloc(allocator, fpath, std.math.maxInt(usize));
            const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
            _ = try cdb.write(trimmed);
        }
        _ = try cdb.write("]");
    }
};

fn addTooling(b: *std.Build, config: struct {
    cdb_gen: *CdbGenerator,
    conch_tests: ?*std.Build.Step.Compile,
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

    if (findProgram(b, "cloc")) |cloc| {
        try addClocStep(b, .{
            .tooling_sources = tooling_sources,
            .cloc = cloc,
        });
    }

    if (config.conch_tests) |conch_tests| {
        try addCoverageStep(b, conch_tests);
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
    const zig_paths: []const []const u8 = &.{ "build.zig", b.pathJoin(&.{ "tests", "main.zig" }) };
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
    if (b.args) |args| {
        cppcheck.addArgs(args);
    }

    const installed_cppcheck_cache_path = getCacheRelativePath(b, &.{"cppcheck"});
    cppcheck.addPrefixedFileArg("--project=", config.cdb_gen.getCdbPath());
    const cppcheck_cache = cppcheck.addPrefixedOutputDirectoryArg(
        "--cppcheck-build-dir=",
        installed_cppcheck_cache_path,
    );
    cppcheck.addArgs(&.{ "--error-exitcode=1", "--enable=all" });
    cppcheck.addArgs(&.{ "-icatch_amalgamated.cpp", "--suppress=*:catch_amalgamated.hpp" });

    const suppressions: []const []const u8 = comptime &.{
        "checkersReport",
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
    };

    inline for (suppressions) |suppression| {
        cppcheck.addArg("--suppress=" ++ suppression);
    }

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

fn addClocStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    cloc: []const u8,
}) !void {
    const cloc = b.addSystemCommand(&.{config.cloc});
    cloc.addFileArg(b.path("src"));
    cloc.addFileArg(b.path("include"));
    const cloc_step = b.step("cloc", "Count lines of code in the src and include directories");
    cloc_step.dependOn(&cloc.step);

    const cloc_all = b.addSystemCommand(&.{config.cloc});
    cloc_all.addArgs(config.tooling_sources);
    cloc_all.addFileArg(b.path("build.zig"));
    const cloc_all_step = b.step("cloc-all", "Count all lines of code including the tests and build script");
    cloc_all_step.dependOn(&cloc_all.step);
}

fn addCoverageStep(b: *std.Build, tests: *std.Build.Step.Compile) !void {
    const error_msg =
        \\Code coverage reporting requires kcov, a free FreeBSD/Linux/MacOS tool:
        \\  https://github.com/SimonKagstrom/kcov
        \\
        \\It is also availble via homebrew with 'brew install kcov'
    ;

    const kcov_step = b.step("cov", "Use kcov to generate code coverage report");
    switch (builtin.os.tag) {
        .freebsd, .linux, .macos => {},
        else => return kcov_step.addError(error_msg, .{}),
    }

    if (findProgram(b, "kcov")) |kcov| {
        const kcov_command = b.addSystemCommand(&.{kcov});
        const include_patterns = try std.mem.join(b.allocator, ",", &.{
            try b.build_root.join(b.allocator, &.{"src"}),
            try b.build_root.join(b.allocator, &.{"include"}),
        });

        kcov_command.addArg(try std.fmt.allocPrint(
            b.allocator,
            "--include-pattern={s}",
            .{include_patterns},
        ));
        kcov_command.addArg(getPrefixRelativePath(b, &.{CoverageParser.coverage_install_dirname}));
        kcov_command.addArtifactArg(tests);
        kcov_step.dependOn(&kcov_command.step);
        kcov_step.dependOn(b.getInstallStep());

        const coverage_parser: *CoverageParser = .init(b, tests);
        coverage_parser.step.dependOn(&kcov_command.step);
        kcov_step.dependOn(&coverage_parser.step);
    } else {
        try kcov_step.addError(error_msg, .{});
    }
}

const CoverageParser = struct {
    const coverage_install_dirname = "coverage";
    const cov_json_filename = "coverage.json";
    const cov_svg_filename = "coverage.svg";

    const CoverageInfo = struct {
        percent_covered: []const u8,
    };
    const ParsedCovInfo = std.json.Parsed(CoverageInfo);

    step: std.Build.Step,
    test_artifact: *std.Build.Step.Compile,

    pub fn init(b: *std.Build, test_artifact: *std.Build.Step.Compile) *CoverageParser {
        const self = b.allocator.create(CoverageParser) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "coverage-parse",
                .owner = b,
                .makeFn = coverageParse,
            }),
            .test_artifact = test_artifact,
        };
        return self;
    }

    fn coverageParse(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *CoverageParser = @fieldParentPtr("step", step);

        const b = step.owner;
        const allocator = b.allocator;

        const installed_cov_json_path = getPrefixRelativePath(b, &.{
            coverage_install_dirname,
            self.test_artifact.name,
            cov_json_filename,
        });

        const content = try b.build_root.handle.readFileAlloc(
            allocator,
            installed_cov_json_path,
            std.math.maxInt(usize),
        );

        const parsed: ParsedCovInfo = try std.json.parseFromSlice(
            CoverageInfo,
            allocator,
            content,
            .{ .ignore_unknown_fields = true },
        );

        const precise_percentage = parsed.value.percent_covered;
        const last_dot = std.mem.lastIndexOfScalar(u8, precise_percentage, '.');
        const percentage = if (last_dot) |dot| precise_percentage[0..dot] else precise_percentage;

        const badge_path = getPrefixRelativePath(b, &.{ coverage_install_dirname, cov_svg_filename });
        const url_str = try std.fmt.allocPrint(
            allocator,
            "https://img.shields.io/badge/Coverage-{s}%25-pink",
            .{percentage},
        );

        const curl_bin = findProgram(b, "curl") orelse return error.CurlNotFound;
        var child: std.process.Child = .init(&.{ curl_bin, "-o", badge_path, url_str }, allocator);
        child.stderr_behavior = .Ignore;

        const term = try child.spawnAndWait();
        switch (term) {
            .Exited => |code| if (code != 0) return error.CurlFailed,
            else => return error.CurlTerminatedAbnormally,
        }
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
    version: []const u8,
}) !void {
    const package_step = b.step("package", "Build the artifacts for packaging");

    const uncompressed_package_dir: []const []const u8 = &.{ "package", "uncompressed" };
    const compressed_package_dir: []const []const u8 = &.{ "package", "compressed" };

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

        std.debug.assert(artifacts.libcatch2 == null);
        std.debug.assert(artifacts.conch_tests == null);
        std.debug.assert(artifacts.cppcheck == null);

        try configurePackArtifacts(b, .{
            .artifacts = &.{ artifacts.libconch, artifacts.conch },
            .target = resolved_target,
            .version = config.version,
        });

        const package_artifact_dirname = try std.fmt.allocPrint(
            b.allocator,
            "conch-{s}-{s}",
            .{ try query.zigTriple(b.allocator), config.version },
        );

        const package_artifact_dir_path = b.pathJoin(uncompressed_package_dir ++ .{package_artifact_dirname});
        const package_options: std.Build.Step.InstallArtifact.Options = .{
            .dest_dir = .{
                .override = .{ .custom = package_artifact_dir_path },
            },
        };

        const platform = b.addInstallArtifact(
            artifacts.conch,
            package_options,
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

            const legal_paths: []const []const []const u8 = &.{
                &.{"LICENSE"},
                &.{"README.md"},
                &.{ ".github", "CHANGELOG.md" },
            };

            var file_installs: std.ArrayList(*std.Build.Step) = .empty;
            for (legal_paths) |path| {
                const install_file_step = b.addInstallFile(
                    b.path(b.pathJoin(path)),
                    b.pathJoin(&.{ package_artifact_dir_path, path[path.len - 1] }),
                );
                package_step.dependOn(&install_file_step.step);
                try file_installs.append(b.allocator, &install_file_step.step);
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
                for (file_installs.items) |step| {
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
            for (file_installs.items) |step| {
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

    return paths.toOwnedSlice(b.allocator);
}

fn collectToolingFiles(b: *std.Build) ![]const []const u8 {
    return std.mem.concat(b.allocator, []const u8, &.{
        try collectFiles(b, "src", .{}),
        try collectFiles(b, "include", .{ .allowed_extensions = &.{".hpp"} }),
        try collectFiles(b, "tests", .{
            .dropped_files = &.{
                "catch_amalgamated.cpp",
                "catch_amalgamated.hpp",
            },
            .allowed_extensions = &.{ ".hpp", ".cpp" },
        }),
    });
}

/// Resolves the relative path with its root at the cache directory
fn getCacheRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    return b.cache_root.join(b.allocator, paths) catch @panic("OOM");
}

fn getRelativeFromRoot(b: *std.Build, root: []const u8, paths: []const []const u8) []const u8 {
    const total_path = std.mem.concat(
        b.allocator,
        []const u8,
        &.{ &.{root}, paths },
    ) catch @panic("OOM");
    return b.pathJoin(total_path);
}

/// Resolves the relative path with its root at the vendored cppcheck directory
fn getCppcheckRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    const cppcheck_root = b.pathJoin(&.{ "vendor", "cppcheck" });
    return getRelativeFromRoot(b, cppcheck_root, paths);
}

/// Resolves the relative path with its root at the installation directory
fn getPrefixRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    const prefix_root = std.fs.path.basename(b.install_prefix);
    return getRelativeFromRoot(b, prefix_root, paths);
}

/// Searches the system PATH for the cmd, returning the binary path if found
fn findProgram(b: *std.Build, cmd: []const u8) ?[]const u8 {
    return b.findProgram(&.{cmd}, &.{}) catch null;
}
