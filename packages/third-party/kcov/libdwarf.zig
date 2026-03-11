const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");

const libdwarf = @import("sources/libdwarf.zig");

const version: std.SemanticVersion = .{
    .major = 2,
    .minor = 3,
    .patch = 1,
};
const version_str = std.fmt.comptimePrint("{f}", .{version});

/// Compiles libdwarf from source as a static library.
/// https://github.com/davea42/libdwarf-code
pub fn build(b: *std.Build, config: Config) ?Dependency {
    const upstream_dep = b.lazyDependency("libdwarf", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    // Generated with reference to CMake
    const config_header = b.addConfigHeader(.{ .style = .blank }, .{
        .HAVE_UNISTD_H = 1,
        .HAVE_SYS_TYPES_H = 1,
        .HAVE_STDINT_H = 1,
        .HAVE_STDDEF_H = 1,
        .HAVE_FCNTL_H = 1,
        .HAVE_SYS_STAT_H = 1,
        .HAVE_FULL_MMAP = 1,

        .PACKAGE_NAME = "libdwarf",
        .PACKAGE_VERSION = version_str,
        .PACKAGE_STRING = "libdwarf " ++ version_str,
        .PACKAGE_BUGREPORT = "https://github.com/davea42/libdwarf-code/issues",
        .PACKAGE_URL = "https://github.com/davea42/libdwarf-code.git",

        .WORDS_BIGENDIAN = if (target.result.cpu.arch.endian() == .big) @as(i32, 1) else null,
    });

    if (upstream_dep) |upstream| {
        const root = upstream.path(libdwarf.root);
        mod.addIncludePath(root);
        mod.addCSourceFiles(.{
            .root = root,
            .files = &libdwarf.sources,
        });
        mod.addConfigHeader(config_header);

        const zlib_dep = zlib.build(b, config);
        mod.linkLibrary(zlib_dep.artifact);
        const zstd_dep = zstd.build(b, config);
        mod.linkLibrary(zstd_dep.artifact);

        const lib = b.addLibrary(.{
            .name = "dwarf",
            .root_module = mod,
        });
        lib.installConfigHeader(config_header);
        lib.installHeadersDirectory(root, "libdwarf", .{});
        return .{ .upstream = upstream, .artifact = lib };
    } else return null;
}
