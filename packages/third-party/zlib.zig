const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const zlib = @import("sources/zlib.zig");

/// Compiles zlib from source as a static library
/// https://github.com/allyourcodebase/zlib
pub fn build(b: *std.Build, config: Config) Dependency {
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
        .files = &zlib.sources,
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
    return .{ .upstream = upstream, .artifact = lib };
}
