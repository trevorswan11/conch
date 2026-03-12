const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const elfutils = @import("sources/elfutils.zig");
const argp = @import("sources/argp.zig");

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");

const Metadata = struct {
    upstream: *std.Build.Dependency,
    config: Config,
    config_header: *std.Build.Step.ConfigHeader = undefined,
};

const Self = @This();

b: *std.Build,
metadata: Metadata,

libargp: Artifact = undefined,
libeu: Artifact = undefined,
libelf: Artifact = undefined,
libdwelf: Artifact = undefined,
libebl: Artifact = undefined,
libdw: Artifact = undefined,

/// Compiles elfutils from source as a static library.
/// Only available on linux.
///
/// https://github.com/allyourcodebase/elfutils
pub fn build(b: *std.Build, config: Config) ?*Self {
    const libargp = buildArgp(b, config);
    const upstream = b.lazyDependency("elfutils", .{});
    if (libargp == null or upstream == null) return null;

    const self = b.allocator.create(Self) catch @panic("OOM");
    self.* = .{
        .b = b,
        .metadata = .{
            .upstream = upstream.?,
            .config = config,
            .config_header = elfutils.configHeader(b, config),
        },
        .libargp = libargp.?,
    };

    self.libeu = self.buildEu();
    self.libelf = self.buildElf();
    self.libdwelf = self.buildDwelf();
    self.libebl = self.buildEbl();
    self.libdw = self.buildDw();
    return self;
}

/// Compiles argp-standalone from source as a static library.
/// https://github.com/allyourcodebase/argp-standalone
fn buildArgp(b: *std.Build, config: Config) ?Artifact {
    const upstream_dep = b.lazyDependency("argp", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

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

    const config_header = argp.configHeader(b, .{
        .target = target,
        .is_gnu_lib_c_version_2_3 = is_gnu_lib_c_version_2_3,
        .have_mempcpy = have_mempcpy,
        .have_strchrnul = have_strchrnul,
        .have_strndup = have_strndup,
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
            .files = &argp.sources,
        });

        const lib = b.addLibrary(.{
            .name = "argp",
            .root_module = mod,
        });
        lib.installHeader(upstream.path("argp.h"), "argp.h");
        return lib;
    } else return null;
}

fn buildEu(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    mod.addConfigHeader(self.metadata.config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    if (target.result.isWasiLibC()) {
        mod.addCMacro("_WASI_EMULATED_MMAN", "1");
        mod.linkSystemLibrary("wasi-emulated-mman", .{});
    }

    const root = self.metadata.upstream.path("lib");
    mod.addIncludePath(root);
    mod.addCSourceFiles(.{
        .root = root,
        .files = elfutils.libeu_sources,
    });

    if (!target.result.isGnuLibC()) {
        mod.linkLibrary(self.libargp);
    }

    return b.addLibrary(.{
        .name = "eu",
        .root_module = mod,
    });
}

fn buildElf(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const root = self.metadata.upstream.path("libelf");
    mod.linkLibrary(self.libeu);
    mod.addConfigHeader(self.metadata.config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    mod.addIncludePath(self.metadata.upstream.path("lib"));
    mod.addIncludePath(root);
    mod.addCSourceFiles(.{
        .root = root,
        .files = elfutils.libelf_sources,
    });

    if (target.result.isWasiLibC()) {
        mod.addCMacro("_WASI_EMULATED_MMAN", "1");
        mod.linkSystemLibrary("wasi-emulated-mman", .{});
    }

    const zlib_dep = zlib.build(b, self.metadata.config);
    mod.linkLibrary(zlib_dep.artifact);
    const zstd_dep = zstd.build(b, self.metadata.config);
    mod.linkLibrary(zstd_dep.artifact);

    const lib = b.addLibrary(.{
        .name = "elf",
        .root_module = mod,
    });
    lib.installHeader(root.path(b, "libelf.h"), "libelf.h");
    lib.installHeader(root.path(b, "gelf.h"), "gelf.h");
    lib.installHeader(root.path(b, "nlist.h"), "nlist.h");
    return lib;
}

fn buildDwelf(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const upstream = self.metadata.upstream;
    const root = upstream.path("libdwelf");
    mod.addConfigHeader(self.metadata.config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    mod.addIncludePath(root);
    mod.addIncludePath(upstream.path("libelf"));
    mod.addIncludePath(upstream.path("libdw"));
    mod.addIncludePath(upstream.path("libdwfl"));
    mod.addIncludePath(upstream.path("libebl"));
    mod.addIncludePath(upstream.path("lib"));
    mod.addCSourceFiles(.{
        .root = root,
        .files = elfutils.libdwelf_sources,
    });
    if (target.result.isWasiLibC()) {
        mod.addCMacro("_WASI_EMULATED_MMAN", "1");
        mod.linkSystemLibrary("wasi-emulated-mman", .{});
    }

    const lib = b.addLibrary(.{
        .name = "dwelf",
        .root_module = mod,
    });
    lib.installHeader(root.path(b, "libdwelf.h"), "libdwelf.h");
    return lib;
}

fn buildEbl(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const upstream = self.metadata.upstream;
    const root = upstream.path("libebl");
    mod.addConfigHeader(self.metadata.config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    mod.addIncludePath(root);
    mod.addIncludePath(upstream.path("libelf"));
    mod.addIncludePath(upstream.path("libdw"));
    mod.addIncludePath(upstream.path("libasm"));
    mod.addIncludePath(upstream.path("lib"));
    mod.addCSourceFiles(.{
        .root = root,
        .files = elfutils.libebl_sources,
    });
    if (target.result.isWasiLibC()) {
        mod.addCMacro("_WASI_EMULATED_MMAN", "1");
        mod.linkSystemLibrary("wasi-emulated-mman", .{});
    }

    const lib = b.addLibrary(.{
        .name = "ebl",
        .root_module = mod,
    });
    lib.installHeader(root.path(b, "libebl.h"), "libebl.h");
    return lib;
}

fn buildDw(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const upstream = self.metadata.upstream;
    const root = upstream.path("libdw");
    mod.linkLibrary(self.libeu);
    mod.linkLibrary(self.libelf);
    mod.linkLibrary(self.libdwelf);
    mod.linkLibrary(self.libebl);
    mod.addConfigHeader(self.metadata.config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    mod.addIncludePath(root);
    mod.addIncludePath(upstream.path("libelf"));
    mod.addIncludePath(upstream.path("libebl"));
    mod.addIncludePath(upstream.path("libdwelf"));
    mod.addIncludePath(upstream.path("lib"));
    mod.addCSourceFiles(.{
        .root = root,
        .files = elfutils.libdw_sources,
    });
    if (target.result.isWasiLibC()) {
        mod.addCMacro("_WASI_EMULATED_MMAN", "1");
        mod.linkSystemLibrary("wasi-emulated-mman", .{});
    }

    const lib = b.addLibrary(.{
        .name = "dw",
        .root_module = mod,
    });
    lib.installHeader(root.path(b, "libdw.h"), "elfutils/libdw.h");
    lib.installHeader(b.path("packages/third-party/kcov/gen/known-dwarf.h"), "elfutils/known-dwarf.h");
    lib.installHeader(root.path(b, "dwarf.h"), "dwarf.h");
    return lib;
}
