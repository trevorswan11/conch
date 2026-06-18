const std = @import("std");

const parent_build = @import("../build.zig");
const Dependency = parent_build.Dependency;
const Config = Dependency.Config;

const replxx = @import("sources/replxx.zig");

/// Compiles replxx from source as a static library
/// https://github.com/AmokHuginnsson/replxx
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("replxx", .{});
    const src_path = upstream.path("src");
    const include_path = upstream.path("include");

    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });

    mod.addCSourceFiles(.{
        .root = src_path,
        .files = &replxx.sources,
        .flags = &.{ "-std=c++11", "-DREPLXX_STATIC" },
    });
    mod.addIncludePath(include_path);
    mod.addIncludePath(src_path);

    const lib = b.addLibrary(.{
        .name = "replxx",
        .root_module = mod,
    });
    lib.installHeader(include_path.path(b, replxx.header), replxx.header);
    return .{ .upstream = upstream, .artifact = lib };
}
