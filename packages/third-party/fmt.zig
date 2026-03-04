const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

/// Compiles fmt from source as a static library
/// https://github.com/fmtlib/fmt
pub fn build(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
}) Dependency {
    const upstream = b.dependency("fmt", .{});
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });

    const include = upstream.path("include");
    mod.addCSourceFiles(.{
        .root = upstream.path("src"),
        .files = &.{ "format.cc", "os.cc" },
        .flags = &.{"-std=c++23"},
    });
    mod.addIncludePath(include);

    const lib = b.addLibrary(.{
        .name = "fmt",
        .root_module = mod,
    });
    lib.installHeadersDirectory(include, "", .{});

    return .{
        .upstream = upstream,
        .artifact = lib,
    };
}
