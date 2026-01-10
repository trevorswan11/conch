const std = @import("std");
const builtin = @import("builtin");

const cdb_frags_base_dirname = "cdb-frags";
const cdb_filename = "compile_commands.json";
var cdb_parent: []const u8 = undefined;
var cdb_frags_path: []const u8 = undefined;

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root = try std.fs.cwd().realpathAlloc(b.allocator, ".");
    cdb_parent = b.pathJoin(&.{ root, b.cache_root.path orelse ".zig-cache" });

    cdb_frags_path = b.pathJoin(&.{ cdb_parent, cdb_frags_base_dirname });
    try std.fs.cwd().makePath(cdb_frags_path);
    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;

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

    try addArtifacts(b, .{
        .target = target,
        .optimize = optimize,
        .flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
    });

    // Tooling
    const tooling_sources = try std.mem.concat(b.allocator, []const u8, &.{
        try getCXXFiles(b, "src", .{}),
        try getCXXFiles(b, "include", .{ .allowed_extensions = &.{".hpp"} }),
        try getCXXFiles(b, "tests", .{
            .dropped_files = &.{
                "catch_amalgamated.cpp",
                "catch_amalgamated.hpp",
            },
            .allowed_extensions = &.{ ".hpp", ".cpp" },
        }),
    });

    // Fmt
    var fmt: *std.Build.Step.Run = undefined;
    if (commandExists(b, "clang-format", "--version")) {
        fmt = b.addSystemCommand(&.{ "clang-format", "-i" });
        fmt.addArgs(tooling_sources);
        const fmt_step = b.step("fmt", "Format all project files");
        fmt_step.dependOn(&fmt.step);

        const fmt_check = b.addSystemCommand(&.{ "clang-format", "--dry-run", "--Werror" });
        fmt_check.addArgs(tooling_sources);
        const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
        fmt_check_step.dependOn(&fmt_check.step);
        try cdb_steps.append(b.allocator, &fmt.step);
    }

    // CDB
    const cdb_step = b.step("cdb", "Compile CDB fragments into " ++ cdb_filename);
    cdb_step.makeFn = collectCDB;
    for (cdb_steps.items) |step| {
        cdb_step.dependOn(step);
    }
    b.getInstallStep().dependOn(cdb_step);

    // Tidy
    var tidy: *std.Build.Step.Run = undefined;
    var has_tidy = false;
    if (builtin.os.tag == .macos and commandExists(b, "run-clang-tidy", "-h")) {
        tidy = b.addSystemCommand(&.{ "run-clang-tidy", "-p", cdb_parent });
        has_tidy = true;
    } else if (commandExists(b, "clang-tidy", "--version")) {
        tidy = b.addSystemCommand(&.{ "clang-tidy", "-p", cdb_parent });
        has_tidy = true;
    }

    if (has_tidy) {
        tidy.addArgs(tooling_sources);
        const tidy_step = b.step("tidy", "Run static analysis on all project files");
        tidy_step.dependOn(&tidy.step);
        tidy_step.dependOn(cdb_step);
    }
}

fn addArtifacts(b: *std.Build, opts: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    flags: []const []const u8,
    cdb_steps: *std.ArrayList(*std.Build.Step),
}) !void {
    // Library for exe and tests
    const lib_mod = b.addModule("libconch", .{
        .link_libcpp = true,
        .target = opts.target,
        .optimize = opts.optimize,
    });
    lib_mod.addIncludePath(b.path("include"));
    lib_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, "src", .{ .dropped_files = &.{"main.cpp"} }),
        .flags = opts.flags,
        .language = .cpp,
    });
    const libconch = b.addLibrary(.{
        .name = "conch",
        .root_module = lib_mod,
    });
    b.installArtifact(libconch);
    try opts.cdb_steps.append(b.allocator, &libconch.step);

    // Exe
    const main = b.pathJoin(&.{ "src", "main.cpp" });
    const conch_mod = b.createModule(.{
        .link_libcpp = true,
        .target = opts.target,
        .optimize = opts.optimize,
    });
    conch_mod.addIncludePath(b.path("include"));
    conch_mod.addCSourceFile(.{
        .file = b.path(main),
        .flags = opts.flags,
        .language = .cpp,
    });
    conch_mod.linkLibrary(libconch);
    const conch = b.addExecutable(.{
        .name = "conch",
        .root_module = conch_mod,
    });
    b.installArtifact(conch);
    try opts.cdb_steps.append(b.allocator, &conch.step);

    const run_cmd = b.addRunArtifact(conch);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run conch with provided command line arguments");
    run_step.dependOn(&run_cmd.step);

    // Catch2
    const catch2_dir = b.pathJoin(&.{ "tests", "test_framework" });
    const catch_mod = b.addModule("libcatch2", .{
        .link_libcpp = true,
        .target = opts.target,
        .optimize = opts.optimize,
    });
    catch_mod.addIncludePath(b.path(catch2_dir));
    catch_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, catch2_dir, .{}),
        .flags = opts.flags,
        .language = .cpp,
    });
    const libcatch2 = b.addLibrary(.{
        .name = "catch2",
        .root_module = catch_mod,
    });
    b.installArtifact(libcatch2);
    try opts.cdb_steps.append(b.allocator, &libcatch2.step);

    // Tests
    const helper_dir = b.pathJoin(&.{ "tests", "helpers" });
    const test_mod = b.createModule(.{
        .root_source_file = b.path("tests/runner.zig"),
        .link_libcpp = true,
        .target = opts.target,
        .optimize = opts.optimize,
    });

    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path(catch2_dir));
    test_mod.addIncludePath(b.path(helper_dir));

    test_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, "tests", .{ .dropped_files = &.{"catch_amalgamated.cpp"} }),
        .flags = opts.flags,
        .language = .cpp,
    });

    test_mod.linkLibrary(libconch);
    test_mod.linkLibrary(libcatch2);
    const conch_tests = b.addExecutable(.{
        .name = "conch_tests",
        .root_module = test_mod,
    });
    b.installArtifact(conch_tests);
    try opts.cdb_steps.append(b.allocator, &conch_tests.step);

    const run_tests = b.addRunArtifact(conch_tests);
    run_tests.step.dependOn(b.getInstallStep());

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_tests.step);
}

fn getCXXFiles(
    b: *std.Build,
    directory: []const u8,
    options: struct {
        allowed_extensions: []const []const u8 = &.{".cpp"},
        dropped_files: ?[]const [:0]const u8 = null,
    },
) ![][]const u8 {
    const cwd = std.fs.cwd();
    var dir = try cwd.openDir(directory, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    var paths: std.ArrayList([]const u8) = .empty;
    collector: while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        for (options.allowed_extensions) |ext| {
            if (std.mem.endsWith(u8, entry.basename, ext)) {
                break;
            }
        } else continue :collector;

        if (!std.mem.endsWith(u8, entry.basename, ".cpp")) continue;
        if (options.dropped_files) |drop| for (drop) |drop_file| {
            if (std.mem.eql(u8, drop_file, entry.path)) continue :collector;
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
    var iter = dir.iterate();

    // Clang generates hashed updates, so grab the most recent for the CDB
    while (try iter.next()) |entry| {
        if (entry.kind != .file) continue;
        const stat = try dir.statFile(entry.name);
        const first_dot = std.mem.indexOf(u8, entry.name, ".").?;
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

    var it = newest_frags.valueIterator();
    var first = true;
    const cdb_path = b.pathJoin(&.{ cdb_parent, cdb_filename });
    const cdb = try std.fs.cwd().createFile(cdb_path, .{});
    defer cdb.close();

    _ = try cdb.write("[");
    while (it.next()) |info| {
        if (!first) _ = try cdb.write(",\n");
        first = false;

        const fpath = b.pathJoin(&.{ cdb_frags_path, info.name });
        const contents = try std.fs.cwd().readFileAlloc(allocator, fpath, std.math.maxInt(usize));
        const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
        _ = try cdb.write(trimmed);
    }
    _ = try cdb.write("]");
}

fn commandExists(b: *std.Build, cmd: []const u8, probe: []const u8) bool {
    const result = std.process.Child.run(.{
        .allocator = b.allocator,
        .argv = &.{ cmd, probe },
    }) catch return false;

    return result.term == .Exited and result.term.Exited == 0;
}
