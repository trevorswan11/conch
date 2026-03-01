const std = @import("std");

const Dependency = @import("Dependency.zig");

const version: std.SemanticVersion = .{
    .major = 2,
    .minor = 15,
    .patch = 1,
};
const version_str = std.fmt.comptimePrint("{f}", .{version});
const version_num = 21501;

/// Compiles libxml2 from source as a static library
/// https://github.com/allyourcodebase/libxml2
pub fn build(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    zlib: Dependency,
}) Dependency {
    const upstream = b.dependency("libxml2", .{});
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
    });
    const os_tag = config.target.result.os.tag;

    // CMake generates this required file usually
    const config_header = b.addConfigHeader(.{
        .style = .{
            .cmake = upstream.path("config.h.cmake.in"),
        },
        .include_path = "config.h",
    }, .{
        .HAVE_STDLIB_H = 1,
        .HAVE_STDINT_H = 1,
        .HAVE_STAT = 1,
        .HAVE_FSTAT = 1,
        .HAVE_FUNC_ATTRIBUTE_DESTRUCTOR = 1,
        .HAVE_LIBHISTORY = 0,
        .HAVE_LIBREADLINE = 0,
        .XML_SYSCONFDIR = 0,

        // Platform-specific logic
        .HAVE_DLOPEN = @intFromBool(os_tag != .windows),
        .XML_THREAD_LOCAL = switch (os_tag) {
            .windows => "__declspec(thread)",
            else => "_Thread_local",
        },
    });
    mod.addConfigHeader(config_header);

    // Autotools generates this required file usually
    const xmlversion_header = b.addConfigHeader(.{
        .style = .{
            .autoconf_at = upstream.path("include/libxml/xmlversion.h.in"),
        },
        .include_path = "libxml/xmlversion.h",
    }, .{
        .VERSION = version_str,
        .LIBXML_VERSION_NUMBER = version_num,
        .LIBXML_VERSION_EXTRA = "-conch",
        .WITH_THREADS = 1,
        .WITH_THREAD_ALLOC = 1,
        .WITH_OUTPUT = 1,
        .WITH_PUSH = 1,
        .WITH_READER = 1,
        .WITH_PATTERN = 1,
        .WITH_WRITER = 1,
        .WITH_SAX1 = 1,
        .WITH_HTTP = 0,
        .WITH_VALID = 1,
        .WITH_HTML = 1,
        .WITH_C14N = 0,
        .WITH_CATALOG = 0,
        .WITH_XPATH = 1,
        .WITH_XPTR = 1,
        .WITH_XINCLUDE = 1,
        .WITH_ICONV = 0,
        .WITH_ICU = 0,
        .WITH_ISO8859X = 1,
        .WITH_DEBUG = 1,
        .WITH_REGEXPS = 1,
        .WITH_RELAXNG = 0,
        .WITH_SCHEMAS = 0,
        .WITH_SCHEMATRON = 0,
        .WITH_MODULES = 0,
        .WITH_ZLIB = 1,
        .MODULE_EXTENSION = config.target.result.dynamicLibSuffix(),
    });
    mod.addConfigHeader(xmlversion_header);

    mod.addCSourceFiles(.{
        .root = upstream.path("."),
        .files = &sources,
        .flags = &.{ "-std=c11", "-D_REENTRANT" },
    });
    mod.addIncludePath(upstream.path("include"));
    mod.addIncludePath(config.zlib.upstream.path("."));

    mod.linkLibrary(config.zlib.artifact);

    const lib = b.addLibrary(.{
        .name = "xml2",
        .root_module = mod,
    });
    lib.installConfigHeader(xmlversion_header);
    lib.installHeadersDirectory(upstream.path("include/libxml"), "libxml", .{});

    return .{
        .upstream = upstream,
        .artifact = lib,
    };
}

const sources = [_][]const u8{
    "buf.c",
    "chvalid.c",
    "dict.c",
    "entities.c",
    "encoding.c",
    "error.c",
    "globals.c",
    "hash.c",
    "list.c",
    "parser.c",
    "parserInternals.c",
    "SAX2.c",
    "threads.c",
    "tree.c",
    "uri.c",
    "valid.c",
    "xmlIO.c",
    "xmlmemory.c",
    "xmlstring.c",
};
