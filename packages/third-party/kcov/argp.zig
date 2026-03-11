const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;

/// Compiles argp-standalone from source as a static library.
/// https://github.com/allyourcodebase/argp-standalone
pub fn build(b: *std.Build, config: Config) ?Dependency {
    const upstream_dep = b.lazyDependency("argp", .{});
    const target = config.target;

    const is_gnu_lib_c_version_2_3 = target.result.isGnuLibC() and target.result.os.isAtLeast(
        .linux,
        .{ .major = 2, .minor = 3, .patch = 0 },
    ) orelse false;

    const have_strchrnul = is_gnu_lib_c_version_2_3;
    const have_strndup = if (target.result.isGnuLibC())
        is_gnu_lib_c_version_2_3
    else
        target.result.os.tag != .windows;
    const have_mempcpy = target.result.os.tag == .windows;

    const config_header = b.addConfigHeader(.{
        .style = .{ .cmake = b.path("packages/third-party/kcov/sources/argp-config.h.in") },
        .include_path = "config.h",
    }, .{
        .HAVE_CONFIG_H = true,
        .HAVE_UNISTD_H = true,
        .HAVE_ALLOCA_H = target.result.os.tag != .windows,
        .HAVE_EX_USAGE = target.result.os.tag != .windows,
        .HAVE_DECL_FLOCKFILE = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_DECL_FPUTS_UNLOCKED = false,
        .HAVE_DECL_FPUTC_UNLOCKED = target.result.os.tag != .windows and !target.result.os.tag.isDarwin(),
        .HAVE_DECL_FWRITE_UNLOCKED = target.result.os.tag != .windows and !target.result.os.tag.isDarwin(),
        .HAVE_DECL_PUTC_UNLOCKED = target.result.os.tag != .windows,
        .HAVE_MEMPCPY = have_mempcpy,
        .HAVE_ASPRINTF = if (target.result.isGnuLibC())
            is_gnu_lib_c_version_2_3
        else
            target.result.os.tag != .windows,
        .HAVE_STRCHRNUL = have_strchrnul,
        .HAVE_STRNDUP = have_strndup,
        .HAVE_DECL_PROGRAM_INVOCATION_NAME = false,
        .HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME = false,
    });

    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    if (upstream_dep) |upstream| {
        const root = upstream.path("");
        if (!have_strchrnul) mod.addCSourceFile(.{ .file = root.path(b, "strchrnul.c") });
        if (!have_strndup) mod.addCSourceFile(.{ .file = root.path(b, "strndup.c") });
        if (!have_mempcpy) mod.addCSourceFile(.{ .file = root.path(b, "mempcpy.c") });

        mod.addConfigHeader(config_header);
        mod.addCMacro("HAVE_CONFIG_H", "1");
        mod.addIncludePath(root);
        mod.addCSourceFiles(.{
            .root = root,
            .files = &.{
                "argp-ba.c",
                "argp-eexst.c",
                "argp-fmtstream.c",
                "argp-help.c",
                "argp-parse.c",
                "argp-pv.c",
                "argp-pvh.c",
            },
        });

        const lib = b.addLibrary(.{
            .name = "argp",
            .root_module = mod,
        });
        lib.installHeader(upstream.path("argp.h"), "argp.h");
        return .{ .upstream = upstream, .artifact = lib };
    } else return null;
}
