const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const iberty = @import("sources/iberty.zig");
const opcodes = @import("sources/opcodes.zig");
const sframe = @import("sources/sframe.zig");
const bfd = @import("sources/bfd.zig");

const VectorArchitectures = @import("sources/VectorArchitectures.zig");

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");

pub const version: std.SemanticVersion = .{
    .major = 2,
    .minor = 45,
    .patch = 0,
};
pub const version_str = std.fmt.comptimePrint("{f}", .{version});

const Self = @This();

const Metadata = struct {
    upstream: *std.Build.Dependency,
    config: Config,
    include: std.Build.LazyPath,
};

b: *std.Build,
metadata: Metadata,

vector_archs: VectorArchitectures,

libsframe: Artifact = undefined,
libiberty: Artifact = undefined,
libbfd: Artifact = undefined,
libopcodes: Artifact = undefined,

/// Compiles binutils from source as a static library.
/// https://github.com/allyourcodebase/binutils
pub fn build(b: *std.Build, config: Config) ?Self {
    const upstream = b.lazyDependency("binutils", .{}) orelse return null;
    const target = config.target;
    var self: Self = .{
        .b = b,
        .vector_archs = .init(target),
        .metadata = .{
            .upstream = upstream,
            .config = config,
            .include = upstream.path("include"),
        },
    };

    self.libsframe = self.buildLibsframe();
    self.libiberty = self.buildLibiberty();
    const libbfd, const bfd_configs = self.buildLibbfd();
    self.libbfd = libbfd;
    self.libopcodes = self.buildLibopcodes(bfd_configs.bfd);

    return self;
}

fn buildLibsframe(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;

    const config = sframe.configHeader(b, target);

    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });
    const root = self.metadata.upstream.path("libsframe");

    const include = self.metadata.include;
    mod.addConfigHeader(config);
    mod.addIncludePath(root);
    mod.addIncludePath(include);
    mod.addIncludePath(self.metadata.upstream.path("libctf"));
    mod.addCSourceFiles(.{
        .root = root,
        .files = &.{
            "sframe.c",
            "sframe-dump.c",
            "sframe-error.c",
        },
    });

    const lib = b.addLibrary(.{
        .name = "sframe",
        .root_module = mod,
    });
    lib.installHeader(include.path(b, "sframe.h"), "sframe.h");
    lib.installHeader(include.path(b, "sframe-api.h"), "sframe-api.h");
    lib.installHeader(include.path(b, "ansidecl.h"), "ansidecl.h");
    lib.setVersionScript(root.path(b, "libsframe.ver"));
    return lib;
}

fn buildLibiberty(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;

    const root = self.metadata.upstream.path("libiberty");
    const config = iberty.configHeader(b, .{ .autoconf_undef = root.path(b, "config.in") }, target);

    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const include = self.metadata.include;
    mod.addConfigHeader(config);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("_GNU_SOURCE", "1");
    mod.addIncludePath(root);
    mod.addIncludePath(include);
    mod.addCSourceFiles(.{
        .root = root,
        .files = &iberty.sources,
    });

    const lib = b.addLibrary(.{
        .name = "iberty",
        .root_module = mod,
    });
    lib.installHeader(include.path(b, "ansidecl.h"), "ansidecl.h");
    lib.installHeader(include.path(b, "demangle.h"), "demangle.h");
    lib.installHeader(include.path(b, "dyn-string.h"), "dyn-string.h");
    lib.installHeader(include.path(b, "fibheap.h"), "fibheap.h");
    lib.installHeader(include.path(b, "floatformat.h"), "floatformat.h");
    lib.installHeader(include.path(b, "hashtab.h"), "hashtab.h");
    lib.installHeader(include.path(b, "libiberty.h"), "libiberty.h");
    lib.installHeader(include.path(b, "objalloc.h"), "objalloc.h");
    lib.installHeader(include.path(b, "partition.h"), "partition.h");
    lib.installHeader(include.path(b, "safe-ctype.h"), "safe-ctype.h");
    lib.installHeader(include.path(b, "sort.h"), "sort.h");
    lib.installHeader(include.path(b, "splay-tree.h"), "splay-tree.h");
    lib.installHeader(include.path(b, "timeval-utils.h"), "timeval-utils.h");
    return lib;
}

fn buildLibbfd(self: *const Self) struct { Artifact, bfd.ConfigHeaders } {
    const b = self.b;
    const config = self.metadata.config;
    const target = config.target;

    const root = self.metadata.upstream.path("bfd");
    const configs = bfd.configHeaders(b, .{
        .config = .{ .autoconf_undef = root.path(b, "config.in") },
        .bfd = .{ .autoconf_at = root.path(b, "bfd-in2.h") },
        .bfdver = .{ .autoconf_at = root.path(b, "version.h") },
    }, target);

    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    const include = self.metadata.include;
    mod.addConfigHeader(configs.config);
    mod.addConfigHeader(configs.bfd);
    mod.addConfigHeader(configs.bfdver);

    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("DEBUGDIR", b.fmt("\"{s}\"", .{b.pathJoin(&.{ b.install_prefix, "lib", "bfdebug" })}));
    mod.linkLibrary(self.libiberty);
    mod.linkLibrary(self.libsframe);
    mod.addIncludePath(root);
    mod.addIncludePath(include);
    mod.addIncludePath(b.path("packages/third-party/kcov/sources/"));
    mod.addCSourceFiles(.{
        .root = root,
        .files = &bfd.sources,
    });

    bfd.generateHeaders(b, root, mod);

    const zlib_dep = zlib.build(b, config);
    mod.linkLibrary(zlib_dep.artifact);
    const zstd_dep = zstd.build(b, config);
    mod.linkLibrary(zstd_dep.artifact);

    const lib = b.addLibrary(.{
        .name = "bfd",
        .root_module = mod,
    });
    lib.installConfigHeader(configs.bfd);
    lib.installHeader(include.path(b, "bfdlink.h"), "bfdlink.h");
    lib.installHeader(include.path(b, "ansidecl.h"), "ansidecl.h");
    lib.installHeader(include.path(b, "symcat.h"), "symcat.h");
    lib.installHeader(include.path(b, "diagnostics.h"), "diagnostics.h");

    bfd.generateSources(b, root, mod, std.mem.concat(b.allocator, []const u8, &.{
        &.{self.vector_archs.default_vector},
        self.vector_archs.select_vectors,
    }) catch @panic("OOM"));

    for (self.vector_archs.select_architectures) |select_architecture| {
        var arch_source = select_architecture;
        arch_source = std.mem.replaceOwned(u8, b.allocator, arch_source, "bfd_", "cpu-") catch @panic("OOM");
        arch_source = std.mem.replaceOwned(u8, b.allocator, arch_source, "_arch", ".c") catch @panic("OOM");
        arch_source = std.mem.replaceOwned(u8, b.allocator, arch_source, "mn10200", "m10200") catch @panic("OOM");
        arch_source = std.mem.replaceOwned(u8, b.allocator, arch_source, "mn10300", "m10300") catch @panic("OOM");
        mod.addCSourceFile(.{ .file = root.path(b, arch_source) });
    }

    mod.addCMacro(b.fmt("HAVE_{s}", .{self.vector_archs.default_vector}), "1");
    for (self.vector_archs.select_vectors) |select_vector| {
        mod.addCMacro(b.fmt("HAVE_{s}", .{select_vector}), "1");
    }

    mod.addCSourceFiles(.{
        .root = root,
        .files = &.{ "targets.c", "archures.c" },
        .flags = &.{
            b.fmt("-DDEFAULT_VECTOR={s}", .{self.vector_archs.default_vector}),
            if (self.vector_archs.select_vectors.len == 0)
                "-DSELECT_VECS=''"
            else
                b.fmt("-DSELECT_VECS=&{s},&{s}", .{
                    self.vector_archs.default_vector,
                    std.mem.join(b.allocator, ",&", self.vector_archs.select_vectors) catch @panic("OOM"),
                }),
            if (self.vector_archs.select_architectures.len == 0)
                "-DSELECT_ARCHITECTURES=''"
            else
                b.fmt("-DSELECT_ARCHITECTURES=&{s}", .{
                    std.mem.join(b.allocator, ",&", self.vector_archs.select_architectures) catch @panic("OOM"),
                }),
        },
    });

    return .{ lib, configs };
}

fn buildLibopcodes(self: *const Self, bfd_header: *std.Build.Step.ConfigHeader) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;

    const root = self.metadata.upstream.path("opcodes");
    const opcodes_config_header = opcodes.configHeader(b, .{ .autoconf_undef = root.path(b, "config.in") }, target);

    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    const include = self.metadata.include;
    mod.addConfigHeader(opcodes_config_header);
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addIncludePath(root);
    mod.addIncludePath(self.metadata.upstream.path("bfd"));
    mod.addIncludePath(include);
    mod.addConfigHeader(bfd_header);
    mod.addCSourceFiles(.{
        .root = root,
        .files = &.{ "dis-buf.c", "dis-init.c" },
    });

    var opcodes_arch_defines: std.ArrayList([]const u8) = .empty;
    for (self.vector_archs.select_architectures) |select_architecture| {
        var arch_define = select_architecture;
        arch_define = std.mem.replaceOwned(u8, b.allocator, arch_define, "bfd_", "") catch @panic("OOM");
        arch_define = std.mem.replaceOwned(u8, b.allocator, arch_define, "_arch", "") catch @panic("OOM");
        opcodes_arch_defines.append(b.allocator, b.fmt("-D{s}=1", .{arch_define})) catch @panic("OOM");

        mod.addCSourceFiles(.{
            .root = root,
            .files = opcodes.sources.get(select_architecture) orelse std.debug.panic("missing target sources for '{s}'", .{select_architecture}),
        });
    }

    mod.addCSourceFile(.{
        .file = root.path(b, "disassemble.c"),
        .flags = opcodes_arch_defines.items,
    });

    const lib = b.addLibrary(.{
        .name = "opcodes",
        .root_module = mod,
    });
    lib.installHeader(include.path(b, "dis-asm.h"), "dis-asm.h");
    return lib;
}
