const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;

const mbedtls = @import("sources/mbedtls.zig");

pub const version: std.SemanticVersion = .{
    .major = 3,
    .minor = 46,
    .patch = 0,
};
pub const version_str = std.fmt.comptimePrint("{f}", .{version});

/// Compiles mbedtls from source as a static library.
/// https://github.com/allyourcodebase/mbedtls
pub fn build(b: *std.Build, config: Config) ?Dependency {
    const upstream_dep = b.lazyDependency("mbedtls", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    if (upstream_dep) |upstream| {
        mod.addIncludePath(upstream.path("include"));
        mod.addCSourceFiles(.{
            .root = upstream.path("library"),
            .files = &mbedtls.sources,
        });

        if (target.result.os.tag == .freebsd) {
            mod.addCMacro("__BSD_VISIBLE", "1");
        }
        mod.addCMacro("MBEDTLS_THREADING_C", "");
        mod.addCMacro("MBEDTLS_THREADING_PTHREAD", "");

        if (target.result.os.tag == .windows) {
            mod.linkSystemLibrary("bcrypt", .{});
        }

        const lib = b.addLibrary(.{
            .name = "mbedtls",
            .root_module = mod,
        });
        lib.installHeadersDirectory(upstream.path("include/mbedtls"), "mbedtls", .{});
        lib.installHeadersDirectory(upstream.path("include/psa"), "psa", .{});
        return .{ .upstream = upstream, .artifact = lib };
    } else return null;
}
