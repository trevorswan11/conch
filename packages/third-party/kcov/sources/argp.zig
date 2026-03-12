//! https://github.com/allyourcodebase/argp-standalone
const std = @import("std");

pub fn configHeader(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    is_gnu_lib_c_version_2_3: bool,
    have_mempcpy: bool,
    have_strchrnul: bool,
    have_strndup: bool,
}) *std.Build.Step.ConfigHeader {
    const target = config.target;
    return b.addConfigHeader(.{
        .style = .{ .cmake = b.path("packages/third-party/kcov/gen/argp-config.h.in") },
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
        .HAVE_MEMPCPY = config.have_mempcpy,
        .HAVE_ASPRINTF = if (target.result.isGnuLibC())
            config.is_gnu_lib_c_version_2_3
        else
            target.result.os.tag != .windows,
        .HAVE_STRCHRNUL = config.have_strchrnul,
        .HAVE_STRNDUP = config.have_strndup,
        .HAVE_DECL_PROGRAM_INVOCATION_NAME = false,
        .HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME = false,
    });
}

pub const sources = [_][]const u8{
    "argp-ba.c",
    "argp-eexst.c",
    "argp-fmtstream.c",
    "argp-help.c",
    "argp-parse.c",
    "argp-pv.c",
    "argp-pvh.c",
};
