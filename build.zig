const std = @import("std");
const builtin = @import("builtin");

const cdb_frags = "cdb-frags";

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const cdb_frags_path = try std.fs.path.join(b.allocator, &.{ b.install_prefix, cdb_frags });
    const base_flags = [_][]const u8{
        "-std=c++23",
        "-gen-cdb-fragment-path",
        cdb_frags_path,
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
    };
    const extra_flags = switch (optimize) {
        .Debug => [_][]const u8{
            "-g", "-DDEBUG",
        },
        .ReleaseSafe => [_][]const u8{
            "-DRELEASE", "-DSAFE"
        },
        .ReleaseFast, .ReleaseSmall => [_][]const u8{
            "-DNDEBUG", "-DDIST"
        },
    };
    const standard_flags = base_flags ++ extra_flags;
    
    std.fs.cwd().access(cdb_frags_path, .{}) catch {
        try std.fs.cwd().makePath(cdb_frags_path);
    };

    // Library for exe and tests
    const lib_mod = b.addModule("libconch", .{
        .link_libcpp = true,
        .target = target,
        .optimize = optimize,
    });
    lib_mod.addIncludePath(b.path("include"));
    lib_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, "src", .{ .dropped_files = &.{"main.cpp"} }),
        .flags = &standard_flags,
        .language = .cpp,
    });
    const libconch = b.addLibrary(.{
        .name = "conch",
        .root_module = lib_mod,
    });
    b.installArtifact(libconch);

    // Exe
    const exe_mod = b.createModule(.{
        .link_libcpp = true,
        .target = target,
        .optimize = optimize,
    });
    exe_mod.addIncludePath(b.path("include"));
    exe_mod.addCSourceFile(.{
        .file = b.path("src/main.cpp"),
        .flags = &standard_flags,
        .language = .cpp,
    });
    exe_mod.linkLibrary(libconch);
    const exe = b.addExecutable(.{
        .name = "conch",
        .root_module = exe_mod,
    });
    b.installArtifact(exe);

    // Catch2
    const catch_mod = b.addModule("libcatch2", .{
        .link_libcpp = true,
        .target = target,
        .optimize = optimize,
    });
    catch_mod.addIncludePath(b.path("tests/test_framework"));
    catch_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, "tests/test_framework", .{}),
        .flags = &standard_flags,
        .language = .cpp,
    });
    const libcatch2 = b.addLibrary(.{
        .name = "catch2",
        .root_module = catch_mod,
    });
    b.installArtifact(libcatch2);

    // Tests
    const test_mod = b.createModule(.{
        .link_libcpp = true,
        .target = target,
        .optimize = optimize,
    });

    test_mod.addIncludePath(b.path("include"));
    test_mod.addIncludePath(b.path("tests/test_framework"));
    test_mod.addIncludePath(b.path("tests/helpers"));

    test_mod.addCSourceFiles(.{
        .files = try getCXXFiles(b, "tests", .{ .dropped_files = &.{"test_framework/catch_amalgamated.cpp"} }),
        .flags = &standard_flags,
        .language = .cpp,
    });
    test_mod.linkLibrary(libconch);
    test_mod.linkLibrary(libcatch2);
    const conch_tests = b.addExecutable(.{
        .name = "conch_tests",
        .root_module = test_mod,
    });
    b.installArtifact(conch_tests);

    const run_tests = b.addRunArtifact(conch_tests);
    run_tests.step.dependOn(b.getInstallStep());

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_tests.step);

    // Asan
    switch (builtin.os.tag) {
        .macos => {},
        .linux => {},
        else => {},
    }

    // CDB
    const cdb_step = b.step("cdb", "Compile CDB fragments into compile_commands.json");
    cdb_step.makeFn = collectCDB;
    cdb_step.dependOn(&libconch.step);
    cdb_step.dependOn(&exe.step);
    cdb_step.dependOn(&libcatch2.step);
    cdb_step.dependOn(&conch_tests.step);
    b.getInstallStep().dependOn(cdb_step);

    // Tooling
    const tooling_sources = try std.mem.concat(b.allocator, []const u8, &.{
        try getCXXFiles(b, "src", .{}),
        try getCXXFiles(b, "include", .{ .allowed_extensions = &.{".hpp"} }),
        try getCXXFiles(b, "tests", .{
            .dropped_files = &.{
                "test_framework/catch_amalgamated.cpp",
                "test_framework/catch_amalgamated.hpp",
            },
            .allowed_extensions = &.{ ".hpp", ".cpp" },
        }),
    });

    // Fmt
    if (commandExists(b, "clang-format", "--version")) {
        const fmt = b.addSystemCommand(&.{ "clang-format", "-i" });
        fmt.addArgs(tooling_sources);
        const fmt_step = b.step("fmt", "Format all project files");
        fmt_step.dependOn(&fmt.step);

        const fmt_check = b.addSystemCommand(&.{ "clang-format", "--dry-run", "--Werror" });
        fmt_check.addArgs(tooling_sources);
        const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
        fmt_check_step.dependOn(&fmt_check.step);
    }

    // Tidy
    if (commandExists(b, "run-clang-tidy", "-h")) {
        const tidy = b.addSystemCommand(&.{ "run-clang-tidy", "-p", b.install_prefix, "" });
        tidy.addArgs(tooling_sources);
        const tidy_step = b.step("tidy", "Format all project files");
        tidy_step.dependOn(&tidy.step);
    }
}

const CollectorOptions = struct {
    allowed_extensions: []const []const u8 = &.{".cpp"},
    dropped_files: ?[]const [:0]const u8 = null,
};

fn getCXXFiles(
    b: *std.Build,
    directory: []const u8,
    options: CollectorOptions,
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

        const full_path = try std.fs.path.join(b.allocator, &.{ directory, entry.path });
        try paths.append(b.allocator, full_path);
    }

    return paths.toOwnedSlice(b.allocator);
}

const FragInfo = struct {
    name: []const u8,
    mtime: i128,
};

fn collectCDB(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
    const allocator = step.owner.allocator;
    var newest_frags: std.StringHashMap(FragInfo) = .init(allocator);
    defer {
        var deinit_it = newest_frags.iterator();
        while (deinit_it.next()) |kv| {
            allocator.free(kv.key_ptr.*);
            allocator.free(kv.value_ptr.name);
        }
        newest_frags.deinit();
    }

    const cdb_frags_path = try std.fs.path.join(allocator, &.{ step.owner.install_prefix, cdb_frags });
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
    const cdb_path = try std.fs.path.join(allocator, &.{ step.owner.install_prefix, "compile_commands.json" });
    const cdb = try std.fs.cwd().createFile(cdb_path, .{});
    defer cdb.close();

    _ = try cdb.write("[");
    while (it.next()) |info| {
        if (!first) _ = try cdb.write(",\n");
        first = false;

        const fpath = try std.fs.path.join(allocator, &.{ cdb_frags_path, info.name });
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
