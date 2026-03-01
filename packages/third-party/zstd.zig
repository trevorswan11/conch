const std = @import("std");

const Dependency = @import("Dependency.zig");

/// Compiles zstd from source as a static library
/// https://github.com/allyourcodebase/zstd
pub fn build(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
}) Dependency {
    const upstream = b.dependency("zstd", .{});
    const lib_path = upstream.path("lib");

    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    mod.addCSourceFiles(.{
        .root = lib_path,
        .files = &sources,
    });

    if (config.target.result.cpu.arch == .x86_64) {
        mod.addAssemblyFile(upstream.path("lib/decompress/huf_decompress_amd64.S"));
    } else {
        mod.addCMacro("ZSTD_DISABLE_ASM", "1");
    }
    mod.addIncludePath(lib_path);

    const lib = b.addLibrary(.{
        .name = "zstd",
        .root_module = mod,
    });
    lib.installHeader(lib_path.path(b, "zstd.h"), "zstd.h");
    lib.installHeader(lib_path.path(b, "zdict.h"), "zdict.h");
    lib.installHeader(lib_path.path(b, "zstd_errors.h"), "zstd_errors.h");

    return .{
        .upstream = upstream,
        .artifact = lib,
    };
}

const sources = [_][]const u8{
    "common/zstd_common.c",
    "common/threading.c",
    "common/entropy_common.c",
    "common/fse_decompress.c",
    "common/xxhash.c",
    "common/error_private.c",
    "common/pool.c",
    "compress/fse_compress.c",
    "compress/huf_compress.c",
    "compress/zstd_double_fast.c",
    "compress/zstd_compress_literals.c",
    "compress/zstdmt_compress.c",
    "compress/zstd_compress_superblock.c",
    "compress/zstd_opt.c",
    "compress/zstd_compress.c",
    "compress/zstd_compress_sequences.c",
    "compress/hist.c",
    "compress/zstd_ldm.c",
    "compress/zstd_lazy.c",
    "compress/zstd_preSplit.c",
    "compress/zstd_fast.c",
    "decompress/zstd_decompress.c",
    "decompress/huf_decompress.c",
    "decompress/zstd_decompress_block.c",
    "decompress/zstd_ddict.c",
    "dictBuilder/divsufsort.c",
    "dictBuilder/zdict.c",
    "dictBuilder/cover.c",
    "dictBuilder/fastcover.c",
    "deprecated/zbuff_decompress.c",
    "deprecated/zbuff_common.c",
    "deprecated/zbuff_compress.c",
};
