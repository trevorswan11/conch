const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const cppcheck = @import("sources/cppcheck.zig");

/// Compiles cppcheck from source using the flags given by:
/// https://github.com/danmar/cppcheck#g-for-experts
pub fn build(b: *std.Build, config: Config) !Dependency {
    const upstream = b.dependency("cppcheck", .{});
    const includes: []const std.Build.LazyPath = &.{
        upstream.path("externals"),
        upstream.path("externals/simplecpp"),
        upstream.path("externals/tinyxml2"),
        upstream.path("externals/picojson"),
        upstream.path("lib"),
        upstream.path("frontend"),
    };

    const root = upstream.path(".");
    const target = config.target;
    const optimize = config.optimize;

    // The path needs to be fixed on windows due to cppcheck internals
    const cfg_path = blk: {
        const raw_cfg_path = try root.getPath3(b, null).toString(b.allocator);
        if (target.result.os.tag == .windows) {
            break :blk try std.mem.replaceOwned(u8, b.allocator, raw_cfg_path, "\\", "/");
        }
        break :blk raw_cfg_path;
    };

    const files_dir = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{cfg_path},
    );

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libcpp = true,
    });

    for (includes) |include| {
        mod.addIncludePath(include);
    }

    mod.addCSourceFiles(.{
        .root = root,
        .files = &cppcheck.sources,
        .flags = &.{ files_dir, "-Uunix", "-std=c++11" },
    });

    return .{
        .upstream = upstream,
        .artifact = b.addExecutable(.{
            .name = "cppcheck",
            .root_module = mod,
        }),
    };
}
