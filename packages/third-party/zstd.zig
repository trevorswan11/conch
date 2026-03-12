const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const zstd = @import("sources/zstd.zig");

/// Compiles zstd from source as a static library
/// https://github.com/allyourcodebase/zstd
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("zstd", .{});
    const lib_path = upstream.path("lib");

    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    mod.addCSourceFiles(.{
        .root = lib_path,
        .files = &zstd.sources,
    });

    if (config.target.result.cpu.arch == .x86_64) {
        mod.addAssemblyFile(upstream.path("lib/decompress/huf_decompress_amd64.S"));
    } else {
        mod.addCMacro("ZSTD_DISABLE_ASM", "1");
    }
    mod.addIncludePath(lib_path);

    const lib = b.addLibrary(.{
        .name = "zstd",
        .root_module = mod,
    });
    lib.installHeader(lib_path.path(b, "zstd.h"), "zstd.h");
    lib.installHeader(lib_path.path(b, "zdict.h"), "zdict.h");
    lib.installHeader(lib_path.path(b, "zstd_errors.h"), "zstd_errors.h");
    return .{ .upstream = upstream, .artifact = lib };
}
