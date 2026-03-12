const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const libarchive = @import("sources/libarchive.zig");

const zlib = @import("zlib.zig");
const zstd = @import("zstd.zig");

/// Compiles libarchive from source as a static library.
/// https://github.com/allyourcodebase/libarchive
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("libarchive", .{});
    const target = config.target;

    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
    });
    const zlib_dep = zlib.build(b, config);
    mod.linkLibrary(zlib_dep.artifact);
    const zstd_dep = zstd.build(b, config);
    mod.linkLibrary(zstd_dep.artifact);

    const config_header = libarchive.configHeader(
        b,
        .{ .cmake = upstream.path("build/cmake/config.h.in") },
        target,
    );
    mod.addConfigHeader(config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");

    const source_root = upstream.path("libarchive");
    mod.addCSourceFiles(.{
        .root = source_root,
        .files = &libarchive.sources,
    });

    if (target.result.os.tag == .windows) {
        mod.linkSystemLibrary("bcrypt", .{});
        mod.addCSourceFiles(.{
            .root = source_root,
            .files = &libarchive.windows_sources,
        });
    } else {
        mod.addCSourceFiles(.{
            .root = source_root,
            .files = &libarchive.unix_sources,
        });
    }

    const lib = b.addLibrary(.{
        .name = "archive",
        .root_module = mod,
    });
    lib.installHeadersDirectory(upstream.path("libarchive"), "", .{});
    lib.installLibraryHeaders(zlib_dep.artifact);
    lib.installLibraryHeaders(zstd_dep.artifact);
    return .{ .upstream = upstream, .artifact = lib };
}
