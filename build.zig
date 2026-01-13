const std = @import("std");
const builtin = @import("builtin");

const cdb_frags_base_dirname = "cdb-frags";
const cdb_filename = "compile_commands.json";
const cppcheck_base_dirname = "cppcheck";
var zig_cache: []const u8 = undefined;
var cdb_frags_path: []const u8 = undefined;
var cppcheck_cache_path: []const u8 = undefined;

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root = try std.fs.cwd().realpathAlloc(b.allocator, ".");
    zig_cache = b.pathJoin(&.{ root, b.cache_root.path orelse ".zig-cache" });

    cdb_frags_path = b.pathJoin(&.{ zig_cache, cdb_frags_base_dirname });
    try std.fs.cwd().makePath(cdb_frags_path);
    cppcheck_cache_path = b.pathJoin(&.{ zig_cache, cppcheck_base_dirname });
    try std.fs.cwd().makePath(cppcheck_cache_path);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-gen-cdb-fragment-path",
        cdb_frags_path,
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
        "-DCATCH_AMALGAMATED_CUSTOM_MAIN",
    });

    switch (optimize) {
        .Debug => try compiler_flags.appendSlice(b.allocator, &.{ "-g", "-DDEBUG" }),
        .ReleaseSafe => try compiler_flags.appendSlice(b.allocator, &.{"-DRELEASE"}),
        .ReleaseFast, .ReleaseSmall => try compiler_flags.appendSlice(b.allocator, &.{ "-DNDEBUG", "-DDIST" }),
    }

    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;
    const artifacts = try addArtifacts(b, .{
        .target = target,
        .optimize = optimize,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
    });
    try addTooling(b, .{ .cdb_steps = cdb_steps, .cppcheck = artifacts.cppcheck });
}

fn addArtifacts(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cxx_flags: []const []const u8,
    cdb_steps: *std.ArrayList(*std.Build.Step),
}) !struct {
    libconch: *std.Build.Step.Compile,
    conch: *std.Build.Step.Compile,
    libcatch2: *std.Build.Step.Compile,
    conch_tests: *std.Build.Step.Compile,
    cppcheck: *std.Build.Step.Compile,
} {
    const libconch = createLibrary(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_path = b.path("include"),
        .cxx_files = try collectFiles(b, "src", .{ .dropped_files = &.{"main.cpp"} }),
        .flags = config.cxx_flags,
    });
    try config.cdb_steps.append(b.allocator, &libconch.step);

    const cxx_main = b.pathJoin(&.{ "src", "main.cpp" });
    const conch = createExecutable(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{b.path("include")},
        .cxx_files = &.{cxx_main},
        .cxx_flags = config.cxx_flags,
        .link_libraries = &.{libconch},
        .behavior = .{
            .runnable = .{
                .cmd_name = "run",
                .cmd_desc = "Run conch with provided command line arguments",
                .with_args = true,
            },
        },
    });
    try config.cdb_steps.append(b.allocator, &conch.step);

    const catch2_dir = b.pathJoin(&.{ "vendor", "catch2" });
    const libcatch2 = createLibrary(b, .{
        .name = "catch2",
        .target = config.target,
        .optimize = config.optimize,
        .include_path = b.path(catch2_dir),
        .cxx_files = try collectFiles(b, catch2_dir, .{}),
        .flags = config.cxx_flags,
    });
    try config.cdb_steps.append(b.allocator, &libcatch2.step);

    const helper_dir = b.pathJoin(&.{ "tests", "helpers" });
    const conch_tests = createExecutable(b, .{
        .name = "conch_tests",
        .zig_main = b.path(b.pathJoin(&.{ "tests", "main.zig" })),
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{ b.path("include"), b.path(catch2_dir), b.path(helper_dir) },
        .cxx_files = try collectFiles(b, "tests", .{}),
        .cxx_flags = config.cxx_flags,
        .link_libraries = &.{ libconch, libcatch2 },
        .behavior = .{
            .runnable = .{
                .cmd_name = "test",
                .cmd_desc = "Run unit tests",
                .with_args = false,
            },
        },
    });
    try config.cdb_steps.append(b.allocator, &conch_tests.step);

    const cppcheck = try compileCppcheck(b, config.target);
    try config.cdb_steps.append(b.allocator, &cppcheck.step);

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
    const cppcheck_root = b.pathJoin(&.{ "vendor", "cppcheck" });
    const cppcheck_incldues: []const std.Build.LazyPath = &.{
        b.path(b.pathJoin(&.{ cppcheck_root, "externals" })),
        b.path(b.pathJoin(&.{ cppcheck_root, "externals", "simplecpp" })),
        b.path(b.pathJoin(&.{ cppcheck_root, "externals", "tinyxml2" })),
        b.path(b.pathJoin(&.{ cppcheck_root, "externals", "picojson" })),
        b.path(b.pathJoin(&.{ cppcheck_root, "lib" })),
        b.path(b.pathJoin(&.{ cppcheck_root, "frontend" })),
    };

    var cppcheck_sources: std.ArrayList([]const u8) = .empty;
    try cppcheck_sources.appendSlice(b.allocator, &.{
        b.pathJoin(&.{ cppcheck_root, "externals", "simplecpp", "simplecpp.cpp" }),
        b.pathJoin(&.{ cppcheck_root, "externals", "tinyxml2", "tinyxml2.cpp" }),
    });

    const cppcheck_glob_srcs: []const []const []const u8 = &.{
        try collectFiles(b, b.pathJoin(&.{ cppcheck_root, "frontend" }), .{}),
        try collectFiles(b, b.pathJoin(&.{ cppcheck_root, "cli" }), .{}),
        try collectFiles(b, b.pathJoin(&.{ cppcheck_root, "lib" }), .{}),
    };

    for (cppcheck_glob_srcs) |glob| {
        try cppcheck_sources.appendSlice(b.allocator, glob);
    }

    const files_dir_define = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{b.pathJoin(&.{ b.install_prefix, "bin" })},
    );

    const cfg_copy = b.addInstallDirectory(.{
        .install_dir = .prefix,
        .source_dir = b.path(b.pathJoin(&.{ "vendor", "cppcheck", "cfg" })),
        .install_subdir = b.pathJoin(&.{ "bin", "cfg" }),
    });

    const cppcheck = createExecutable(b, .{
        .name = "cppcheck",
        .target = target,
        .optimize = .ReleaseFast,
        .include_paths = cppcheck_incldues,
        .cxx_files = cppcheck_sources.items,
        .cxx_flags = &.{files_dir_define},
    });

    cppcheck.step.dependOn(&cfg_copy.step);
    return cppcheck;
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
    b.installArtifact(lib);
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
    behavior: union(enum) {
        runnable: struct {
            cmd_name: []const u8,
            cmd_desc: []const u8,
            with_args: bool,
        },
        standalone: void,
    } = .{ .standalone = {} },
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
    b.installArtifact(exe);

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

fn addTooling(b: *std.Build, config: struct {
    cdb_steps: std.ArrayList(*std.Build.Step),
    cppcheck: *std.Build.Step.Compile,
}) !void {
    // Compile commands
    const cdb_step = b.step("cdb", "Compile CDB fragments into " ++ cdb_filename);
    cdb_step.makeFn = collectCDB;
    for (config.cdb_steps.items) |step| {
        cdb_step.dependOn(step);
    }
    b.getInstallStep().dependOn(cdb_step);

    const tooling_sources = try std.mem.concat(b.allocator, []const u8, &.{
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

    // Fmt
    if (findProgram(b, "clang-format")) |clang_fmt| {
        const build_fmt = b.addFmt(.{ .paths = &.{"build.zig"} });
        const build_fmt_check = b.addFmt(.{ .paths = &.{"build.zig"}, .check = true });

        const fmt = b.addSystemCommand(&.{clang_fmt});
        fmt.addArg("-i");
        fmt.addArgs(tooling_sources);
        const fmt_step = b.step("fmt", "Format all project files");
        fmt_step.dependOn(&fmt.step);
        fmt_step.dependOn(&build_fmt.step);

        const fmt_check = b.addSystemCommand(&.{clang_fmt});
        fmt_check.addArgs(&.{ "--dry-run", "--Werror" });
        fmt_check.addArgs(tooling_sources);
        const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
        fmt_check_step.dependOn(&fmt_check.step);
        fmt_check_step.dependOn(&build_fmt_check.step);
    }

    // Static analysis
    const cppcheck = b.addRunArtifact(config.cppcheck);
    if (b.args) |args| {
        cppcheck.addArgs(args);
    }

    cppcheck.addPrefixedFileArg("--project=", b.path(b.pathJoin(&.{ ".zig-cache", cdb_filename })));
    cppcheck.addPrefixedDirectoryArg(
        "--cppcheck-build-dir=",
        b.path(b.pathJoin(&.{ ".zig-cache", cppcheck_base_dirname })),
    );
    cppcheck.addArgs(&.{ "--error-exitcode=1", "--enable=all" });
    cppcheck.addArgs(&.{ "-icatch_amalgamated.cpp", "--suppress=*:catch_amalgamated.hpp" });

    const suppressions: []const []const u8 = comptime &.{
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
    };

    inline for (suppressions) |suppression| {
        cppcheck.addArg("--suppress=" ++ suppression);
    }

    const check_step = b.step("check", "Run static analysis on all project files");
    check_step.dependOn(&cppcheck.step);
    check_step.dependOn(cdb_step);

    // Cloc
    if (findProgram(b, "cloc")) |cloc_command| {
        const cloc = b.addSystemCommand(&.{cloc_command});
        cloc.addFileArg(b.path("src"));
        cloc.addFileArg(b.path("include"));
        const cloc_step = b.step("cloc", "Count lines of code in the src and include directories");
        cloc_step.dependOn(&cloc.step);

        const cloc_all = b.addSystemCommand(&.{"cloc"});
        cloc_all.addArgs(tooling_sources);
        cloc_all.addFileArg(b.path("build.zig"));
        const cloc_all_step = b.step("cloc-all", "Count all lines of code including the tests and build script");
        cloc_all_step.dependOn(&cloc_all.step);
    }

    const clean_step = b.step("clean", "Clean up emitted artifacts");
    clean_step.dependOn(&b.addRemoveDirTree(b.path("zig-out")).step);
    clean_step.dependOn(&b.addRemoveDirTree(b.path(".zig-cache")).step);
}

fn collectFiles(
    b: *std.Build,
    directory: []const u8,
    options: struct {
        allowed_extensions: []const []const u8 = &.{".cpp"},
        dropped_files: ?[]const [:0]const u8 = null,
    },
) ![]const []const u8 {
    const cwd = std.fs.cwd();
    var dir = try cwd.openDir(directory, .{ .iterate = true });
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

const FragInfo = struct {
    name: []const u8,
    mtime: i128,
};

fn collectCDB(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
    const b = step.owner;
    const allocator = b.allocator;
    var newest_frags: std.StringHashMap(FragInfo) = .init(allocator);

    var dir = try std.fs.cwd().openDir(cdb_frags_path, .{ .iterate = true });
    defer dir.close();
    var dir_iter = dir.iterate();

    // Clang generates hashed updates, so grab the most recent for the CDB
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
    const cdb_path = b.pathJoin(&.{ zig_cache, cdb_filename });
    const cdb = try std.fs.cwd().createFile(cdb_path, .{});
    defer cdb.close();

    _ = try cdb.write("[");
    while (frag_iter.next()) |info| {
        if (!first) _ = try cdb.write(",\n");
        first = false;

        const fpath = b.pathJoin(&.{ cdb_frags_path, info.name });
        const contents = try std.fs.cwd().readFileAlloc(allocator, fpath, std.math.maxInt(usize));
        const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
        _ = try cdb.write(trimmed);
    }
    _ = try cdb.write("]");
}

fn findProgram(b: *std.Build, cmd: []const u8) ?[]const u8 {
    return b.findProgram(&.{cmd}, &.{}) catch null;
}
