//! https://github.com/allyourcodebase/binutils/blob/master/build.zig
const std = @import("std");

pub fn configHeader(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    comptime version_str: []const u8,
) *std.Build.Step.ConfigHeader {
    return b.addConfigHeader(.{}, .{
        .HAVE_BYTESWAP_H = if (target.result.os.tag == .linux or target.result.os.tag == .wasi) true else null,
        .HAVE_DECL_BSWAP_16 = target.result.os.tag == .linux or target.result.os.tag == .wasi,
        .HAVE_DECL_BSWAP_32 = target.result.os.tag == .linux or target.result.os.tag == .wasi,
        .HAVE_DECL_BSWAP_64 = target.result.os.tag == .linux or target.result.os.tag == .wasi,
        .HAVE_DLFCN_H = if (target.result.os.tag != .windows) true else null,
        .HAVE_ENDIAN_H = if (target.result.os.tag == .linux or target.result.os.tag == .wasi) true else null,
        .HAVE_GETPAGESIZE = if (target.result.os.tag != .windows) true else null,
        .HAVE_INTTYPES_H = true,
        .HAVE_MEMORY_H = true,
        .HAVE_MMAP = if (target.result.os.tag == .linux) true else null,
        .HAVE_STDINT_H = true,
        .HAVE_STDLIB_H = true,
        .HAVE_STRINGS_H = true,
        .HAVE_STRING_H = true,
        .HAVE_SYS_PARAM_H = true,
        .HAVE_SYS_STAT_H = true,
        .HAVE_SYS_TYPES_H = true,
        .HAVE_UNISTD_H = true,
        .LT_OBJDIR = ".libs/",
        .PACKAGE = "libsframe",
        .PACKAGE_BUGREPORT = "",
        .PACKAGE_NAME = "libsframe",
        .PACKAGE_STRING = "libsframe ",
        .PACKAGE_TARNAME = "libsframe",
        .PACKAGE_URL = "",
        .PACKAGE_VERSION = version_str,
        .STDC_HEADERS = true,
        .VERSION = version_str,
    });
}
