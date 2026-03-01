const std = @import("std");

const Dependency = @import("Dependency.zig");

/// Compiles zlib from source as a static library
/// https://github.com/allyourcodebase/zlib
pub fn build(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
}) Dependency {
    const upstream = b.dependency("zlib", .{});
    const root = upstream.path(".");
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    var flags: std.ArrayList([]const u8) = .empty;
    flags.appendSlice(b.allocator, &.{ "-std=c11", "-D_REENTRANT" }) catch @panic("OOM");
    if (config.target.result.os.tag != .windows) {
        flags.appendSlice(b.allocator, &.{ "-DHAVE_UNISTD_H", "-DHAVE_SYS_TYPES_H" }) catch @panic("OOM");
    } else {
        flags.append(b.allocator, "-DWIN32") catch @panic("OOM");
    }

    mod.addCSourceFiles(.{
        .root = root,
        .files = &sources,
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
        .upstream = upstream,
        .artifact = lib,
    };
}

const sources = [_][]const u8{
    "adler32.c",
    "crc32.c",
    "deflate.c",
    "infback.c",
    "inffast.c",
    "inflate.c",
    "inftrees.c",
    "trees.c",
    "zutil.c",
    "compress.c",
    "uncompr.c",
    "gzclose.c",
    "gzlib.c",
    "gzread.c",
    "gzwrite.c",
};
