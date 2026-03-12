const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const catch2 = @import("sources/catch2.zig");

/// Compiles catch2 from source as a static library.
/// https://github.com/allyourcodebase/catch2
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("catch2", .{});
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libcpp = true,
    });

    const src = upstream.path("src");
    const source_root = src.path(b, "catch2");
    const user_config = catch2.configHeader(b, .{
        .cmake = source_root.path(b, "catch_user_config.hpp.in"),
    });

    mod.addCSourceFiles(.{
        .root = source_root,
        .files = &catch2.sources,
        .flags = &.{"--std=c++23"},
    });

    mod.addConfigHeader(user_config);
    mod.addIncludePath(src);

    const lib = b.addLibrary(.{
        .name = "catch2",
        .root_module = mod,
    });

    lib.installHeadersDirectory(src, "", .{
        .include_extensions = &.{".hpp"},
    });
    lib.installConfigHeader(user_config);

    return .{
        .upstream = upstream,
        .artifact = lib,
    };
}
