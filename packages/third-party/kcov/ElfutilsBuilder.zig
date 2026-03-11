const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const elfutils = @import("sources/elfutils.zig");

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");
const argp = @import("argp.zig");

const version: std.SemanticVersion = .{
    .major = 0,
    .minor = 192,
    .patch = 0,
};
pub const version_str = std.fmt.comptimePrint("{d}.{d}", .{ version.major, version.major });

const Metadata = struct {
    upstream: *std.Build.Dependency,
    config: Config,
    config_header: *std.Build.Step.ConfigHeader = undefined,
};

const Self = @This();

b: *std.Build,
metadata: Metadata,

libeu: Artifact = undefined,
libelf: Artifact = undefined,
libdwelf: Artifact = undefined,
libebl: Artifact = undefined,
libdw: Artifact = undefined,

/// Compiles elfutils from source as a static library.
/// Only available on linux.
///
/// https://github.com/allyourcodebase/elfutils
pub fn build(b: *std.Build, config: Config) ?Self {
    if (config.target.result.os.tag != .linux) return null;
    const upstream_dep = b.lazyDependency("elfutils", .{});
    const argp_dep = argp.build(b, config);
    if (upstream_dep != null and argp_dep != null) {
        var self: Self = .{
            .b = b,
            .metadata = .{
                .upstream = upstream_dep.?,
                .config = config,
                .config_header = elfutils.configHeader(b, config),
            },
        };

        self.libeu = self.buildEu(argp_dep.?);
        self.libelf = self.buildElf();
        self.libdwelf = self.buildDwelf();
        self.libebl = self.buildEbl();
        self.libdw = self.buildDw();
        return self;
    } else return null;
}

fn buildEu(self: *const Self, argp_dep: Dependency) Artifact {
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
        mod.linkLibrary(argp_dep.artifact);
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
    lib.installHeader(b.path("packages/third-party/kcov/sources/known-dwarf.h"), "elfutils/known-dwarf.h");
    lib.installHeader(root.path(b, "dwarf.h"), "dwarf.h");
    return lib;
}
