const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");
const CurlBuilder = @import("CurlBuilder.zig");
const BinutilsBuilder = @import("BinutilsBuilder.zig");
const ElfutilsBuilder = @import("ElfutilsBuilder.zig");

const dwarf = @import("sources/dwarf.zig");
const kcov = @import("sources/kcov.zig");

const Metadata = struct {
    upstream: *std.Build.Dependency,
    config: Config,
    root: std.Build.LazyPath,
    include: std.Build.LazyPath,
};

const Self = @This();

b: *std.Build,
metadata: Metadata,

curl: *CurlBuilder,
binutils: ?*BinutilsBuilder,
elfutils: ?*ElfutilsBuilder,
libdwarf: ?Artifact,

kcov_sowrapper: Artifact = undefined,
bash_execve_redirector: Artifact = undefined,
bash_tracefd_cloexec: Artifact = undefined,
kcov_system_lib: Artifact = undefined,
kcov_exe: Artifact = undefined,

bin_to_c_source_files: kcov.BinToCSourceFiles = undefined,
signer: ?*std.Build.Step.Run = null,

/// Builds kcov from source.
/// https://github.com/allyourcodebase/kcov
pub fn build(b: *std.Build, config: Config) ?*Self {
    const target = config.target;
    switch (target.result.os.tag) {
        .macos, .linux, .freebsd => {},
        else => return null,
    }

    var binutils: ?*BinutilsBuilder = null;
    const needs_binutils = target.result.cpu.arch.isX86();
    if (needs_binutils) binutils = BinutilsBuilder.build(b, config);
    const awaiting_bu = binutils == null and needs_binutils;

    var elfutils: ?*ElfutilsBuilder = null;
    const needs_elfutils = target.result.os.tag == .linux;
    if (needs_elfutils) elfutils = ElfutilsBuilder.build(b, config);
    const awaiting_eu = elfutils == null and needs_elfutils;

    var libdwarf: ?Artifact = null;
    const needs_libdwarf = target.result.os.tag.isDarwin();
    if (needs_libdwarf) libdwarf = buildDwarf(b, config);
    const awaiting_libdwarf = libdwarf == null and needs_libdwarf;

    const awaiting_platform_specific = awaiting_bu or awaiting_eu or awaiting_libdwarf;
    const curl = CurlBuilder.build(b, config);
    const upstream = b.lazyDependency("kcov", .{});
    if (curl == null or upstream == null or awaiting_platform_specific) return null;

    const self = b.allocator.create(Self) catch @panic("OOM");
    self.* = .{
        .b = b,
        .metadata = .{
            .upstream = upstream.?,
            .config = config,
            .root = upstream.?.path(""),
            .include = upstream.?.path("src/include"),
        },
        .curl = curl.?,
        .binutils = binutils,
        .elfutils = elfutils,
        .libdwarf = libdwarf,
    };

    self.kcov_sowrapper = self.buildKcovSOWrapper();
    self.bash_execve_redirector = self.buildBashExecveRedirector();
    self.bash_tracefd_cloexec = self.buildBashTracefdCloexec();
    self.kcov_system_lib = self.buildKcovSystemLib();
    self.bin_to_c_source_files = kcov.runBinToCSource(self);
    self.kcov_exe = self.buildKcov();

    return self;
}

/// Compiles libdwarf from source as a static library.
/// https://github.com/davea42/libdwarf-code
fn buildDwarf(b: *std.Build, config: Config) ?Artifact {
    const upstream_dep = b.lazyDependency("libdwarf", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    if (upstream_dep) |upstream| {
        const root = upstream.path(dwarf.root);
        mod.addIncludePath(root);
        mod.addCSourceFiles(.{
            .root = root,
            .files = &dwarf.sources,
        });
        const config_header = dwarf.configHeader(b, target);
        mod.addConfigHeader(config_header);

        const zlib_dep = zlib.build(b, config);
        mod.linkLibrary(zlib_dep.artifact);
        const zstd_dep = zstd.build(b, config);
        mod.linkLibrary(zstd_dep.artifact);

        const lib = b.addLibrary(.{
            .name = "dwarf",
            .root_module = mod,
        });
        lib.installConfigHeader(config_header);
        lib.installHeadersDirectory(root, "", .{});
        return lib;
    } else return null;
}

fn buildKcovSOWrapper(self: *const Self) Artifact {
    const b = self.b;
    const mod = b.createModule(.{
        .target = self.metadata.config.target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    mod.addIncludePath(self.metadata.include);
    mod.addCSourceFiles(.{
        .root = self.metadata.root,
        .files = &.{
            "src/solib-parser/phdr_data.c",
            "src/solib-parser/lib.c",
        },
    });

    return b.addLibrary(.{
        .name = "kcov_sowrapper",
        .root_module = mod,
    });
}

fn buildBashExecveRedirector(self: *const Self) Artifact {
    const b = self.b;
    const mod = b.createModule(.{
        .target = self.metadata.config.target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });

    mod.addCSourceFile(.{
        .file = self.metadata.root.path(b, "src/engines/bash-execve-redirector.c"),
    });

    return b.addLibrary(.{
        .name = "bash_execve_redirector",
        .root_module = mod,
    });
}

fn buildBashTracefdCloexec(self: *const Self) Artifact {
    const b = self.b;
    const mod = b.createModule(.{
        .target = self.metadata.config.target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });
    mod.addCSourceFile(.{
        .file = self.metadata.root.path(b, "src/engines/bash-tracefd-cloexec.c"),
    });

    return b.addLibrary(.{
        .name = "bash_tracefd_cloexec",
        .root_module = mod,
    });
}

fn buildKcovSystemLib(self: *const Self) Artifact {
    const b = self.b;
    const mod = b.createModule(.{
        .root_source_file = b.addWriteFiles().add("empty.zig", ""),
        .target = self.metadata.config.target,
        .optimize = self.metadata.config.optimize,
    });

    return b.addLibrary(.{
        .name = "kcov_system_lib",
        .root_module = mod,
    });
}

fn buildKcov(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .pic = true,
        .link_libc = true,
        .link_libcpp = true,
    });

    mod.addIncludePath(self.metadata.include);
    mod.addCMacro("KCOV_LIBRARY_PREFIX", "/tmp");

    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.bash_redirector_library_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.bash_cloexec_library_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.python_helper_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.bash_helper_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.kcov_system_library_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.html_data_files_cc });
    mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.version_c });

    mod.addCSourceFile(.{ .file = self.metadata.root.path(b, "src/writers/coveralls-writer.cc") });

    if (self.binutils) |binutils| {
        mod.linkLibrary(binutils.libbfd);
        mod.linkLibrary(binutils.libopcodes);
        mod.addCSourceFile(.{
            .file = self.metadata.root.path(b, "src/parsers/bfd-disassembler.cc"),
        });
        mod.addCMacro("ATTRIBUTE_FPTR_PRINTF_2", "ATTRIBUTE_FPTR_PRINTF(2, 3)");
        mod.addCMacro("KCOV_HAS_LIBBFD", "1");
        mod.addCMacro("KCOV_LIBFD_DISASM_STYLED", "1");
        mod.addCMacro("PACKAGE", "1");
        mod.addCMacro("PACKAGE_VERSION", "1");
    } else {
        mod.addCSourceFile(.{
            .file = self.metadata.root.path(b, "src/parsers/dummy-disassembler.cc"),
        });
        mod.addCMacro("KCOV_HAS_LIBBFD", "0");
        mod.addCMacro("KCOV_LIBFD_DISASM_STYLED", "0");
    }

    mod.addCSourceFiles(.{
        .root = self.metadata.root.path(b, "src"),
        .files = &kcov.sources,
    });

    switch (target.result.os.tag) {
        .linux, .freebsd => |os_tag| {
            mod.addCSourceFiles(.{
                .root = self.metadata.root,
                .files = &.{
                    "src/engines/ptrace.cc",
                    if (os_tag == .linux)
                        "src/engines/ptrace_linux.cc"
                    else
                        "src/engines/ptrace_freebsd.cc",
                    "src/parsers/elf.cc",
                    "src/parsers/elf-parser.cc",
                    "src/parsers/dwarf.cc",
                    "src/solib-handler.cc",
                    "src/solib-parser/phdr_data.c",
                },
            });

            if (os_tag == .linux) {
                mod.addCSourceFile(.{
                    .file = self.metadata.root.path(b, "src/engines/kernel-engine.cc"),
                });
            }
            mod.addCSourceFile(.{ .file = self.bin_to_c_source_files.library_cc });
        },
        .ios,
        .macos,
        .watchos,
        .tvos,
        => {
            mod.addCSourceFile(.{
                .file = self.metadata.root.path(b, "src/dummy-solib-handler.cc"),
            });
            mod.addCSourceFiles(.{
                .root = self.metadata.root,
                .files = &.{
                    "src/parsers/macho-parser.cc",
                    "src/engines/mach-engine.cc",
                    "src/engines/osx/mach_excServer.c",
                },
            });
        },
        else => |os_tag| std.debug.panic("unsupported os '{s}'", .{@tagName(os_tag)}),
    }

    mod.linkLibrary(self.curl.libcurl);
    CurlBuilder.addFrameworkSearchPaths(mod, target);
    const zlib_dep = zlib.build(b, self.metadata.config);
    mod.linkLibrary(zlib_dep.artifact);

    if (self.elfutils) |elfutils| {
        mod.linkLibrary(elfutils.libelf);
        mod.linkLibrary(elfutils.libdw);
    } else if (self.libdwarf) |libdwarf| {
        mod.linkLibrary(libdwarf);
    }

    return b.addExecutable(.{
        .name = "kcov",
        .root_module = mod,
    });
}

pub const RunKcovConfig = struct {
    include_patterns: ?[]const []const u8 = null,
    exclude_patterns: ?[]const []const u8 = null,
    artifact: Artifact,
};

pub const RunKcovReport = struct {
    runner: *std.Build.Step.Run,
    output_dir: std.Build.LazyPath,
    generated_dirname: []const u8,
};

/// Runs kcov with the given configuration, returning the generated command and directory.
///
/// Can only error on macos if the `codesign` tool is not found.
pub fn runKcov(self: *Self, config: RunKcovConfig) !RunKcovReport {
    const b = self.b;
    const target = self.metadata.config.target;
    if (target.result.os.tag.isDarwin()) {
        const codesign = b.findProgram(&.{"codesign"}, &.{"usr"}) catch return error.CodesignNotFound;
        const run = b.addSystemCommand(&.{codesign});
        run.addArgs(&.{ "-s", "-", "--entitlements" });
        run.addFileArg(self.metadata.root.path(b, "osx-entitlements.xml"));
        run.addArg("-f");
        run.addArtifactArg(self.kcov_exe);

        _ = run.captureStdOut();
        _ = run.captureStdErr();
        self.signer = run;
    }

    const run = b.addRunArtifact(self.kcov_exe);
    if (self.signer) |signer| run.step.dependOn(&signer.step);
    run.has_side_effects = true;
    if (config.include_patterns) |include_patterns| {
        const includes = std.mem.join(b.allocator, ",", include_patterns) catch @panic("OOM");
        run.addArg(b.fmt("--include-pattern={s}", .{includes}));
    }

    if (config.exclude_patterns) |exclude_patterns| {
        const excludes = std.mem.join(b.allocator, ",", exclude_patterns) catch @panic("OOM");
        run.addArg(b.fmt("--exclude-pattern={s}", .{excludes}));
    }

    const gendir = b.fmt("kcov-{s}", .{config.artifact.name});
    const output = run.addOutputDirectoryArg(gendir);
    run.addArtifactArg(config.artifact);
    return .{
        .runner = run,
        .output_dir = output,
        .generated_dirname = gendir,
    };
}

pub const MergeKcovResult = struct {
    runner: *std.Build.Step.Run,
    output_dir: std.Build.LazyPath,
};

pub fn mergeKcovReports(self: *Self, reports: []const RunKcovReport) MergeKcovResult {
    const b = self.b;
    const run = b.addRunArtifact(self.kcov_exe);
    run.addArg("--merge");
    const output = run.addOutputDirectoryArg("kcov-merged");
    for (reports) |report| {
        run.addDirectoryArg(report.output_dir);
    }

    return .{ .runner = run, .output_dir = output };
}
