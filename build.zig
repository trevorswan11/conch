const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .ReleaseFast,
    });

    const flag_opts = addFlagOptions(b);
    const cdb_gen: *CdbGenerator = .init(b);

    var compiler_flags: std.ArrayList([]const u8) = .empty;
    try compiler_flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Wpedantic",
        "-Wno-gnu-statement-expression",
        "-Wno-gnu-statement-expression-from-macro-expansion",
        "-DMAGIC_ENUM_RANGE_MAX=255",
    });

    var package_flags = try compiler_flags.clone(b.allocator);
    const dist_flags: []const []const u8 = &.{ "-DNDEBUG", "-DDIST" };
    try package_flags.appendSlice(b.allocator, dist_flags);

    try compiler_flags.appendSlice(b.allocator, &.{
        "-gen-cdb-fragment-path",
        getCacheRelativePath(b, &.{CdbGenerator.cdb_frags_dirname}),
    });
    switch (optimize) {
        .Debug => try compiler_flags.appendSlice(b.allocator, &.{ "-g", "-DDEBUG" }),
        .ReleaseSafe => try compiler_flags.appendSlice(b.allocator, &.{"-DRELEASE"}),
        .ReleaseFast, .ReleaseSmall => try compiler_flags.appendSlice(b.allocator, dist_flags),
    }
    try compiler_flags.append(b.allocator, "-DCATCH_AMALGAMATED_CUSTOM_MAIN");

    const llvm_config: LLVMConfig = try .init(b);

    var cdb_steps: std.ArrayList(*std.Build.Step) = .empty;
    const artifacts = try addArtifacts(b, .{
        .target = b.graph.host,
        .optimize = optimize,
        .cxx_flags = compiler_flags.items,
        .cdb_steps = &cdb_steps,
        .skip_tests = flag_opts.skip_tests,
        .skip_cppcheck = flag_opts.skip_cppcheck,
        .llvm_config = llvm_config,
    });
    for (cdb_steps.items) |cdb_step| cdb_gen.step.dependOn(cdb_step);

    try addTooling(b, .{
        .cdb_gen = cdb_gen,
        .cli = artifacts.cli,
        .tests = artifacts.tests,
        .cppcheck = artifacts.cppcheck,
        .clean_cache = flag_opts.clean_cache,
    });

    try addPackageStep(b, .{
        .cxx_flags = package_flags.items,
        .compile_only = flag_opts.compile_only,
    });
}

const ProjectPaths = struct {
    const Project = struct {
        inc: []const u8,
        src: []const u8,
        tests: []const u8,

        pub fn files(self: *const Project, b: *std.Build) ![][]const u8 {
            return std.mem.concat(b.allocator, []const u8, &.{
                try collectFiles(b, self.inc, .{}),
                try collectFiles(b, self.src, .{ .allowed_extensions = &.{".hpp"} }),
                try collectFiles(b, self.tests, .{ .allowed_extensions = &.{ ".hpp", ".cpp" } }),
            });
        }
    };

    const compiler: Project = .{
        .inc = "packages/compiler/include/",
        .src = "packages/compiler/src/",
        .tests = "packages/compiler/tests/",
    };

    const cli: Project = .{
        .inc = "apps/cli/include/",
        .src = "apps/cli/src/",
        .tests = "apps/cli/tests/",
    };

    const core: Project = .{
        .inc = "packages/core/include/",
        .src = "packages/core/src/",
        .tests = "packages/core/tests/",
    };

    const stdlib = "packages/stdlib/";
    const test_runner = "packages/test_runner/";
};

const LLVMConfig = struct {
    const whitespace = " \r\n\t";

    const Dependency = struct {
        dependency: *std.Build.Dependency,
        artifact: *std.Build.Step.Compile,
    };

    b: *std.Build,

    llvm_config_path: []const u8,
    version: std.SemanticVersion,

    pub fn init(b: *std.Build) !LLVMConfig {
        const llvm_config_path = findProgram(b, "llvm-config") orelse return error.LLVMNotFound;

        const version_str = b.run(&.{ llvm_config_path, "--version" });
        const version: std.SemanticVersion = try .parse(trimWhitespace(version_str));
        if (version.major != 21) {
            return error.InvalidLLVMVersion;
        }

        return .{
            .b = b,
            .llvm_config_path = llvm_config_path,
            .version = version,
        };
    }

    pub fn getConfig(self: *const LLVMConfig) !struct {
        include_dir: std.Build.LazyPath,
        library_dir: std.Build.LazyPath,
        cpp_flags: []const []const u8,
        link_libraries: []const []const u8,
        system_libraries: []const []const u8,
    } {
        const allocator = self.b.allocator;
        const include_dir = self.run(&.{ "--link-static", "--includedir" });
        const library_dir = self.run(&.{ "--link-static", "--libdir" });
        if (std.mem.containsAtLeastScalar(u8, library_dir, 1, ' ')) {
            return error.LLVMPathWithSpaces;
        }

        // C preprocessor flags
        const cppflags = self.run(&.{ "--link-static", "--cppflags" });
        var cpp_flags: std.ArrayList([]const u8) = .empty;
        var it = tokenizeWhitespace(cppflags);
        while (it.next()) |next| {
            if (std.mem.startsWith(u8, next, "-D")) {
                try cpp_flags.append(allocator, next);
            }
        }

        // All static libraries to link against
        const libs = self.run(&.{ "--link-static", "--libs" });
        const link_libraries = try allocator.alloc([]const u8, std.mem.count(u8, libs, " ") + 1);
        it = tokenizeWhitespace(libs);
        var i: usize = 0;
        while (it.next()) |next| : (i += 1) {
            if (std.mem.startsWith(u8, next, "-l")) {
                link_libraries[i] = next[2..];
            }
        }

        // Required system libraries
        const system_libs = self.run(&.{ "--link-static", "--system-libs" });
        var system_libraries: std.ArrayList([]const u8) = .empty;
        it = tokenizeWhitespace(system_libs);
        i = 0;

        const internally_managed = [_][]const u8{ "xml2", "z" };
        outer: while (it.next()) |next| : (i += 1) {
            const lib_name = next[2..];
            for (internally_managed) |managed| {
                if (std.mem.eql(u8, lib_name, managed)) {
                    continue :outer;
                }
            }

            if (std.mem.startsWith(u8, next, "-l")) {
                try system_libraries.append(allocator, lib_name);
            }
        }

        return .{
            .include_dir = .{ .cwd_relative = trimWhitespace(include_dir) },
            .library_dir = .{ .cwd_relative = trimWhitespace(library_dir) },
            .cpp_flags = cpp_flags.items,
            .link_libraries = link_libraries,
            .system_libraries = system_libraries.items,
        };
    }

    pub fn dependencies(self: *const LLVMConfig, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
        auto_install: bool,
    }) !struct {
        libxml2: Dependency,
        zlib: Dependency,
    } {
        const zlib = try self.compileZLib(.{
            .target = config.target,
            .optimize = config.optimize,
        });

        const libxml2 = self.compileLibXml2(.{
            .target = config.target,
            .optimize = config.optimize,
            .zlib_include = zlib.dependency.path("."),
        });

        const b = self.b;
        if (config.auto_install) {
            b.installArtifact(zlib.artifact);
            b.installArtifact(libxml2.artifact);
        }

        return .{
            .libxml2 = libxml2,
            .zlib = zlib,
        };
    }

    /// Compiles zlib from source as a static library
    /// Reference: https://github.com/allyourcodebase/zlib
    fn compileZLib(self: *const LLVMConfig, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
    }) !Dependency {
        const b = self.b;
        const zlib = b.dependency("zlib", .{});
        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
        });

        const src_files = [_][]const u8{
            "adler32.c", "compress.c", "crc32.c",   "deflate.c",
            "gzclose.c", "gzlib.c",    "gzread.c",  "gzwrite.c",
            "infback.c", "inffast.c",  "inflate.c", "inftrees.c",
            "trees.c",   "uncompr.c",  "zutil.c",
        };

        var flags: std.ArrayList([]const u8) = .empty;
        try flags.appendSlice(b.allocator, &.{ "-std=c11", "-D_REENTRANT" });
        if (config.target.result.os.tag != .windows) {
            try flags.appendSlice(b.allocator, &.{ "-DHAVE_UNISTD_H", "-DHAVE_SYS_TYPES_H" });
        } else {
            try flags.append(b.allocator, "-DWIN32");
        }

        mod.addCSourceFiles(.{
            .root = zlib.path("."),
            .files = &src_files,
            .flags = flags.items,
        });
        mod.addIncludePath(zlib.path("."));

        return .{
            .dependency = zlib,
            .artifact = b.addLibrary(.{
                .name = "z",
                .root_module = mod,
            }),
        };
    }

    /// Compiles libxml2 from source as a static library
    fn compileLibXml2(self: *const LLVMConfig, config: struct {
        target: std.Build.ResolvedTarget,
        optimize: std.builtin.OptimizeMode,
        zlib_include: std.Build.LazyPath,
    }) Dependency {
        const b = self.b;
        const libxml = b.dependency("libxml2", .{});
        const mod = b.createModule(.{
            .target = config.target,
            .optimize = config.optimize,
            .link_libc = true,
        });

        // CMake generates this required file usually
        const config_header = b.addConfigHeader(.{
            .style = .{
                .cmake = libxml.path("config.h.cmake.in"),
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
            .HAVE_DLOPEN = @intFromBool(config.target.result.os.tag != .windows),
            .XML_THREAD_LOCAL = switch (config.target.result.os.tag) {
                .windows => "__declspec(thread)",
                else => "_Thread_local",
            },
        });
        mod.addConfigHeader(config_header);

        // Autotools generates this required file usually
        const xmlversion_header = b.addConfigHeader(.{
            .style = .{
                .autoconf_at = libxml.path("include/libxml/xmlversion.h.in"),
            },
            .include_path = "libxml/xmlversion.h",
        }, .{
            .VERSION = "2.15.1",
            .LIBXML_VERSION_NUMBER = 21501,
            .LIBXML_VERSION_EXTRA = "-conch-static",
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
            .WITH_ICONV = 1,
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
            .root = libxml.path("."),
            .files = &.{
                "buf.c",       "dict.c",      "entities.c", "error.c",           "globals.c",
                "hash.c",      "list.c",      "parser.c",   "parserInternals.c", "SAX2.c",
                "threads.c",   "tree.c",      "uri.c",      "valid.c",           "xmlIO.c",
                "xmlmemory.c", "xmlstring.c",
            },
            .flags = &.{ "-std=c11", "-D_REENTRANT" },
        });
        mod.addIncludePath(libxml.path("include"));
        mod.addIncludePath(config.zlib_include);

        const lib = b.addLibrary(.{
            .name = "xml2",
            .root_module = mod,
        });

        return .{
            .dependency = libxml,
            .artifact = lib,
        };
    }

    fn trimWhitespace(str: []const u8) []const u8 {
        return std.mem.trim(u8, str, whitespace);
    }

    fn tokenizeWhitespace(str: []const u8) std.mem.TokenIterator(u8, .any) {
        return std.mem.tokenizeAny(u8, str, whitespace);
    }

    // Runs llvm-config with the provided args. The result is owned by the build.
    fn run(self: *const LLVMConfig, comptime args: []const []const u8) []u8 {
        return self.b.run(.{self.llvm_config_path} ++ args);
    }
};

fn addFlagOptions(b: *std.Build) struct {
    compile_only: bool,
    skip_tests: bool,
    skip_cppcheck: bool,
    clean_cache: bool,
} {
    const compile_only = b.option(
        bool,
        "compile-only",
        "Skip copying legal documents to and compressing packaged directories",
    ) orelse false;

    const skip_tests = b.option(
        bool,
        "skip-tests",
        "Skip compilation of tests and disable test running",
    ) orelse false;

    const skip_cppcheck = b.option(
        bool,
        "skip-cppcheck",
        "Skip compilation of cppcheck and disable static analysis tooling",
    ) orelse false;

    const clean_cache = builtin.os.tag != .windows and b.option(
        bool,
        "clean-cache",
        "Clean the build cache with the clean step",
    ) orelse false;

    return .{
        .compile_only = compile_only,
        .skip_tests = skip_tests,
        .skip_cppcheck = skip_cppcheck,
        .clean_cache = clean_cache,
    };
}

const ExecutableBehavior = union(enum) {
    runnable: struct {
        cmd_name: []const u8,
        cmd_desc: []const u8,
    },
    standalone: void,
};

const TestArtifacts = struct {
    libcatch2: *std.Build.Step.Compile = undefined,
    runner_tests: *std.Build.Step.Compile = undefined,
    core_tests: *std.Build.Step.Compile = undefined,
    compiler_tests: *std.Build.Step.Compile = undefined,
    cli_tests: *std.Build.Step.Compile = undefined,

    pub fn configure(
        self: *const TestArtifacts,
        b: *std.Build,
        install: bool,
        cdb_steps: ?*std.ArrayList(*std.Build.Step),
    ) !void {
        if (install) {
            b.installArtifact(self.libcatch2);
            b.installArtifact(self.runner_tests);
            b.installArtifact(self.core_tests);
            b.installArtifact(self.compiler_tests);
            b.installArtifact(self.cli_tests);
        }

        if (cdb_steps) |cdb| {
            try cdb.append(b.allocator, &self.libcatch2.step);
            try cdb.append(b.allocator, &self.core_tests.step);
            try cdb.append(b.allocator, &self.compiler_tests.step);
            try cdb.append(b.allocator, &self.cli_tests.step);
        }

        const runners = [_]*std.Build.Step.Run{
            b.addRunArtifact(self.runner_tests),
            b.addRunArtifact(self.core_tests),
            b.addRunArtifact(self.compiler_tests),
            b.addRunArtifact(self.cli_tests),
        };

        const test_step = b.step("test", "Run all unit tests");
        for (runners) |runner| {
            runner.step.dependOn(b.getInstallStep());
            test_step.dependOn(&runner.step);
        }
    }
};

fn addArtifacts(b: *std.Build, config: struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    cxx_flags: []const []const u8,
    cdb_steps: ?*std.ArrayList(*std.Build.Step),
    skip_tests: bool,
    skip_cppcheck: bool,
    llvm_config: LLVMConfig,
    behavior: ?ExecutableBehavior = null,
    auto_install: bool = true,
}) !struct {
    libcore: *std.Build.Step.Compile,
    libcompiler: *std.Build.Step.Compile,
    libcli: *std.Build.Step.Compile,
    cli: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
} {
    const magic_enum = b.dependency("magic_enum", .{});
    const magic_enum_inc = magic_enum.path("include");

    // Shared core functionality
    const libcore = createLibrary(b, .{
        .name = "core",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{ b.path(ProjectPaths.core.inc), magic_enum_inc },
        .cxx_files = try collectFiles(b, ProjectPaths.core.src, .{}),
        .flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libcore);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcore.step);

    // LLVM is only used by the compiler but it has specific deps
    const llvm_config = try config.llvm_config.getConfig();
    const llvm_deps = try config.llvm_config.dependencies(.{
        .optimize = config.optimize,
        .target = config.target,
        .auto_install = config.auto_install,
    });

    // The actual compiler static library
    const libcompiler = createLibrary(b, .{
        .name = "compiler",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
            magic_enum_inc,
            llvm_config.include_dir,
        },
        .link_libraries = &.{
            libcore,
            llvm_deps.libxml2.artifact,
            llvm_deps.zlib.artifact,
        },
        .system_libraries = .{
            .search_paths = &.{llvm_config.library_dir},
            .libs = try std.mem.concat(b.allocator, []const u8, &.{
                llvm_config.link_libraries,
                llvm_config.system_libraries,
            }),
        },
        .cxx_files = try collectFiles(b, ProjectPaths.compiler.src, .{}),
        .flags = try std.mem.concat(b.allocator, []const u8, &.{
            config.cxx_flags,
            llvm_config.cpp_flags,
        }),
    });
    if (config.auto_install) b.installArtifact(libcompiler);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcompiler.step);

    // The CLI library is stripped of main
    const libcli = createLibrary(b, .{
        .name = "cli",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{
            b.path(ProjectPaths.cli.inc),
            b.path(ProjectPaths.compiler.inc),
            b.path(ProjectPaths.core.inc),
            magic_enum_inc,
        },
        .cxx_files = try collectFiles(b, ProjectPaths.cli.src, .{
            .dropped_files = &.{"main.cpp"},
        }),
        .link_libraries = &.{libcompiler},
        .system_libraries = .{
            .search_paths = &.{llvm_config.library_dir},
            .libs = try std.mem.concat(b.allocator, []const u8, &.{
                llvm_config.link_libraries,
                llvm_config.system_libraries,
            }),
        },
        .flags = config.cxx_flags,
    });
    if (config.auto_install) b.installArtifact(libcli);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &libcli.step);

    // The shippable executable links only against libcli which has a transitive dep of the compiler
    const cli = createExecutable(b, .{
        .name = "conch",
        .target = config.target,
        .optimize = config.optimize,
        .include_paths = &.{b.path(ProjectPaths.cli.inc)},
        .cxx_files = &.{ProjectPaths.cli.src ++ "main.cpp"},
        .cxx_flags = config.cxx_flags,
        .link_libraries = &.{libcli},
        .system_libraries = .{
            .search_paths = &.{llvm_config.library_dir},
            .libs = try std.mem.concat(b.allocator, []const u8, &.{
                llvm_config.link_libraries,
                llvm_config.system_libraries,
            }),
        },
        .behavior = config.behavior orelse .{
            .runnable = .{
                .cmd_name = "run",
                .cmd_desc = "Run conch with provided command line arguments",
            },
        },
    });
    if (config.auto_install) b.installArtifact(cli);
    if (config.cdb_steps) |cdb_steps| try cdb_steps.append(b.allocator, &cli.step);

    var tests: ?TestArtifacts = null;
    if (!config.skip_tests) {
        tests = .{};
        const test_runner = b.path(ProjectPaths.test_runner ++ "main.zig");

        const catch2 = b.dependency("catch2", .{});
        tests.?.libcatch2 = createLibrary(b, .{
            .name = "catch2",
            .target = config.target,
            .optimize = .ReleaseSafe,
            .include_paths = &.{catch2.path("extras")},
            .source_root = catch2.path("."),
            .cxx_files = &.{"extras/catch_amalgamated.cpp"},
            .flags = config.cxx_flags,
        });

        // The runner has standalone tests
        tests.?.runner_tests = b.addTest(.{
            .name = "runner_tests",
            .root_module = b.createModule(.{
                .root_source_file = test_runner,
                .optimize = config.optimize,
                .target = config.target,
                .link_libc = true,
            }),
        });

        const runner_cmd = b.addRunArtifact(tests.?.runner_tests);
        const runner_step = b.step("test-runner", "Run test instrumentation tests");
        runner_step.dependOn(&runner_cmd.step);

        // Core tests depend on the test runner
        tests.?.core_tests = createExecutable(b, .{
            .name = "core_tests",
            .zig_main = test_runner,
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.core.tests),
                catch2.path("extras"),
                magic_enum_inc,
            },
            .cxx_files = try collectFiles(b, ProjectPaths.core.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ tests.?.libcatch2, libcore },
            .system_libraries = .{
                .search_paths = &.{llvm_config.library_dir},
                .libs = try std.mem.concat(b.allocator, []const u8, &.{
                    llvm_config.link_libraries,
                    llvm_config.system_libraries,
                }),
            },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-core",
                    .cmd_desc = "Run core unit tests",
                },
            },
        });

        // Compiler tests can pull in core helpers
        tests.?.compiler_tests = createExecutable(b, .{
            .name = "compiler_tests",
            .zig_main = test_runner,
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{ b.path(ProjectPaths.compiler.inc), b.path(ProjectPaths.core.inc), b.path(ProjectPaths.compiler.tests), catch2.path("extras"), magic_enum_inc },
            .cxx_files = try collectFiles(b, ProjectPaths.compiler.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ libcompiler, tests.?.libcatch2 },
            .system_libraries = .{
                .search_paths = &.{llvm_config.library_dir},
                .libs = try std.mem.concat(b.allocator, []const u8, &.{
                    llvm_config.link_libraries,
                    llvm_config.system_libraries,
                }),
            },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-compiler",
                    .cmd_desc = "Run compiler unit tests",
                },
            },
        });

        // CLI tests can pull in core helpers
        tests.?.cli_tests = createExecutable(b, .{
            .name = "cli_tests",
            .zig_main = b.path(ProjectPaths.test_runner ++ "main.zig"),
            .target = config.target,
            .optimize = config.optimize,
            .include_paths = &.{
                b.path(ProjectPaths.compiler.inc),
                b.path(ProjectPaths.cli.inc),
                b.path(ProjectPaths.core.inc),
                b.path(ProjectPaths.cli.tests),
                catch2.path("extras"),
                magic_enum_inc,
            },
            .cxx_files = try collectFiles(b, ProjectPaths.cli.tests, .{
                .extra_files = &.{ProjectPaths.test_runner ++ "runner.cpp"},
            }),
            .cxx_flags = config.cxx_flags,
            .link_libraries = &.{ libcompiler, libcli, tests.?.libcatch2 },
            .system_libraries = .{
                .search_paths = &.{llvm_config.library_dir},
                .libs = try std.mem.concat(b.allocator, []const u8, &.{
                    llvm_config.link_libraries,
                    llvm_config.system_libraries,
                }),
            },
            .behavior = config.behavior orelse .{
                .runnable = .{
                    .cmd_name = "test-cli",
                    .cmd_desc = "Run CLI unit tests",
                },
            },
        });

        try tests.?.configure(b, config.auto_install, config.cdb_steps);
    }

    const cppcheck = if (config.skip_cppcheck) null else try compileCppcheck(b, config.target);

    return .{
        .libcore = libcore,
        .libcompiler = libcompiler,
        .libcli = libcli,
        .cli = cli,
        .tests = tests,
        .cppcheck = cppcheck,
    };
}

/// Compiles cppcheck from source using the flags given by:
/// https://github.com/danmar/cppcheck#g-for-experts
fn compileCppcheck(b: *std.Build, target: std.Build.ResolvedTarget) !*std.Build.Step.Compile {
    const cppcheck = b.dependency("cppcheck", .{});
    const cppcheck_includes: []const std.Build.LazyPath = &.{
        cppcheck.path("externals"),
        cppcheck.path("externals/simplecpp"),
        cppcheck.path("externals/tinyxml2"),
        cppcheck.path("externals/picojson"),
        cppcheck.path("lib"),
        cppcheck.path("frontend"),
    };

    const cppcheck_sources = [_][]const u8{
        "externals/simplecpp/simplecpp.cpp", "externals/tinyxml2/tinyxml2.cpp",
        "frontend/frontend.cpp",             "cli/cmdlineparser.cpp",
        "cli/cppcheckexecutor.cpp",          "cli/executor.cpp",
        "cli/filelister.cpp",                "cli/main.cpp",
        "cli/processexecutor.cpp",           "cli/sehwrapper.cpp",
        "cli/signalhandler.cpp",             "cli/singleexecutor.cpp",
        "cli/stacktrace.cpp",                "cli/threadexecutor.cpp",
        "lib/addoninfo.cpp",                 "lib/analyzerinfo.cpp",
        "lib/astutils.cpp",                  "lib/check.cpp",
        "lib/check64bit.cpp",                "lib/checkassert.cpp",
        "lib/checkautovariables.cpp",        "lib/checkbool.cpp",
        "lib/checkbufferoverrun.cpp",        "lib/checkclass.cpp",
        "lib/checkcondition.cpp",            "lib/checkers.cpp",
        "lib/checkersidmapping.cpp",         "lib/checkersreport.cpp",
        "lib/checkexceptionsafety.cpp",      "lib/checkfunctions.cpp",
        "lib/checkinternal.cpp",             "lib/checkio.cpp",
        "lib/checkleakautovar.cpp",          "lib/checkmemoryleak.cpp",
        "lib/checknullpointer.cpp",          "lib/checkother.cpp",
        "lib/checkpostfixoperator.cpp",      "lib/checksizeof.cpp",
        "lib/checkstl.cpp",                  "lib/checkstring.cpp",
        "lib/checktype.cpp",                 "lib/checkuninitvar.cpp",
        "lib/checkunusedfunctions.cpp",      "lib/checkunusedvar.cpp",
        "lib/checkvaarg.cpp",                "lib/clangimport.cpp",
        "lib/color.cpp",                     "lib/cppcheck.cpp",
        "lib/ctu.cpp",                       "lib/errorlogger.cpp",
        "lib/errortypes.cpp",                "lib/findtoken.cpp",
        "lib/forwardanalyzer.cpp",           "lib/fwdanalysis.cpp",
        "lib/importproject.cpp",             "lib/infer.cpp",
        "lib/keywords.cpp",                  "lib/library.cpp",
        "lib/mathlib.cpp",                   "lib/path.cpp",
        "lib/pathanalysis.cpp",              "lib/pathmatch.cpp",
        "lib/platform.cpp",                  "lib/preprocessor.cpp",
        "lib/programmemory.cpp",             "lib/regex.cpp",
        "lib/reverseanalyzer.cpp",           "lib/sarifreport.cpp",
        "lib/settings.cpp",                  "lib/standards.cpp",
        "lib/summaries.cpp",                 "lib/suppressions.cpp",
        "lib/symboldatabase.cpp",            "lib/templatesimplifier.cpp",
        "lib/timer.cpp",                     "lib/token.cpp",
        "lib/tokenize.cpp",                  "lib/tokenlist.cpp",
        "lib/utils.cpp",                     "lib/valueflow.cpp",
        "lib/vf_analyzers.cpp",              "lib/vf_common.cpp",
        "lib/vf_settokenvalue.cpp",          "lib/vfvalue.cpp",
    };

    // The path needs to be fixed on windows due to cppcheck internals
    const cfg_path = blk: {
        const raw_cfg_path = try cppcheck.path(".").getPath3(b, null).toString(b.allocator);
        if (builtin.os.tag == .windows) {
            break :blk try std.mem.replaceOwned(u8, b.allocator, raw_cfg_path, "\\", "/");
        }
        break :blk raw_cfg_path;
    };

    const files_dir = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{cfg_path},
    );

    return createExecutable(b, .{
        .name = "cppcheck",
        .target = target,
        .optimize = .ReleaseSafe,
        .include_paths = cppcheck_includes,
        .source_root = cppcheck.path("."),
        .cxx_files = &cppcheck_sources,
        .cxx_flags = &.{ files_dir, "-Uunix", "-std=c++11" },
    });
}

const SystemLibraries = struct {
    search_paths: []const std.Build.LazyPath,
    libs: []const []const u8,
};

fn createLibrary(b: *std.Build, config: struct {
    name: []const u8,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    include_paths: []const std.Build.LazyPath,
    source_root: ?std.Build.LazyPath = null,
    link_libraries: ?[]const *std.Build.Step.Compile = null,
    system_libraries: ?SystemLibraries = null,
    cxx_files: []const []const u8,
    flags: []const []const u8,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    for (config.include_paths) |inc_path| {
        mod.addIncludePath(inc_path);
    }

    if (config.link_libraries) |link_libraries| {
        for (link_libraries) |lib| {
            mod.linkLibrary(lib);
        }
    }

    mod.addCSourceFiles(.{
        .root = config.source_root,
        .files = config.cxx_files,
        .flags = config.flags,
        .language = .cpp,
    });

    if (config.system_libraries) |libs| {
        for (libs.search_paths) |path| {
            mod.addLibraryPath(path);
        }

        for (libs.libs) |lib| {
            mod.linkSystemLibrary(lib, .{
                .preferred_link_mode = .static,
            });
        }
    }

    return b.addLibrary(.{
        .name = config.name,
        .root_module = mod,
    });
}

fn createExecutable(b: *std.Build, config: struct {
    name: []const u8,
    zig_main: ?std.Build.LazyPath = null,
    target: ?std.Build.ResolvedTarget,
    optimize: ?std.builtin.OptimizeMode,
    include_paths: []const std.Build.LazyPath,
    source_root: ?std.Build.LazyPath = null,
    cxx_files: []const []const u8,
    cxx_flags: []const []const u8,
    link_libraries: []const *std.Build.Step.Compile = &.{},
    system_libraries: ?SystemLibraries = null,
    behavior: ExecutableBehavior = .standalone,
}) *std.Build.Step.Compile {
    const mod = b.createModule(.{
        .root_source_file = config.zig_main,
        .target = config.target,
        .optimize = config.optimize,
        .link_libc = true,
        .link_libcpp = true,
    });

    for (config.include_paths) |include| {
        mod.addIncludePath(include);
    }

    for (config.link_libraries) |library| {
        mod.linkLibrary(library);
    }

    mod.addCSourceFiles(.{
        .root = config.source_root,
        .files = config.cxx_files,
        .flags = config.cxx_flags,
        .language = .cpp,
    });

    if (config.system_libraries) |libs| {
        for (libs.search_paths) |path| {
            mod.addLibraryPath(path);
        }

        for (libs.libs) |lib| {
            mod.linkSystemLibrary(lib, .{
                .preferred_link_mode = .static,
            });
        }
    }

    const exe = b.addExecutable(.{
        .name = config.name,
        .root_module = mod,
    });

    switch (config.behavior) {
        .runnable => |run| {
            const run_cmd = b.addRunArtifact(exe);
            run_cmd.step.dependOn(b.getInstallStep());

            if (b.args) |args| {
                run_cmd.addArgs(args);
            }

            const run_step = b.step(run.cmd_name, run.cmd_desc);
            run_step.dependOn(&run_cmd.step);
        },
        .standalone => {},
    }

    return exe;
}

const CdbGenerator = struct {
    const cdb_filename = "compile_commands.json";
    const cdb_frags_dirname = "cdb-frags";

    const CdbFileInfo = struct {
        file: []const u8,
    };
    const ParsedCdbFileInfo = std.json.Parsed(CdbFileInfo);

    const FragInfo = struct {
        name: []const u8,
        mtime: i128,
    };

    step: std.Build.Step,
    output_file: std.Build.GeneratedFile,

    pub fn init(b: *std.Build) *CdbGenerator {
        const self = b.allocator.create(CdbGenerator) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "generate-cdb",
                .owner = b,
                .makeFn = generateCdb,
            }),
            .output_file = .{ .step = &self.step },
        };
        return self;
    }

    pub fn getCdbPath(self: *const CdbGenerator) std.Build.LazyPath {
        return .{ .generated = .{ .file = &self.output_file } };
    }

    fn generateCdb(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const self: *CdbGenerator = @fieldParentPtr("step", step);

        const b = step.owner;
        const allocator = b.allocator;
        const cache_root = b.cache_root.handle;

        self.output_file.path = getCacheRelativePath(b, &.{cdb_filename});
        try cache_root.makePath(cdb_frags_dirname);
        var newest_frags: std.StringHashMap(FragInfo) = .init(allocator);

        var dir = try cache_root.openDir(cdb_frags_dirname, .{ .iterate = true });
        defer dir.close();
        var dir_iter = dir.iterate();

        // The frags balloon like crazy so cleaning up proactively is needed
        var old_frags: std.ArrayList([]const u8) = .empty;

        // Hashed updates are generated by the compiler, so grab the most recent for the cdb
        const file_buf = try allocator.alloc(u8, 64 * 1024);
        while (try dir_iter.next()) |entry| {
            if (entry.kind != .file) continue;
            const entry_name = b.dupe(entry.name);
            const stat = try dir.statFile(entry_name);
            const first_dot = std.mem.indexOf(u8, entry_name, ".") orelse {
                try old_frags.append(allocator, entry_name);
                continue;
            };
            const base_name = entry_name[0 .. first_dot + 4];

            const entry_contents = try dir.readFile(entry_name, file_buf);
            const trimmed = std.mem.trimEnd(u8, entry_contents, ",\n\r\t");
            const parsed: ParsedCdbFileInfo = std.json.parseFromSlice(
                CdbFileInfo,
                allocator,
                trimmed,
                .{ .ignore_unknown_fields = true },
            ) catch continue;
            const ref_path = parsed.value.file;
            const absolute_ref_path = if (std.fs.path.isAbsolute(ref_path))
                ref_path
            else
                try b.build_root.join(allocator, &.{ref_path});

            // Orphaned files should be removed too
            std.fs.accessAbsolute(absolute_ref_path, .{}) catch {
                try old_frags.append(allocator, entry_name);
                continue;
            };

            const gop = try newest_frags.getOrPut(base_name);
            if (!gop.found_existing) {
                gop.value_ptr.* = .{
                    .name = entry_name,
                    .mtime = stat.mtime,
                };
            } else {
                if (stat.mtime > gop.value_ptr.mtime) {
                    try old_frags.append(allocator, gop.value_ptr.name);
                    gop.value_ptr.name = entry_name;
                    gop.value_ptr.mtime = stat.mtime;
                } else {
                    try old_frags.append(allocator, entry_name);
                }
            }
        }

        for (old_frags.items) |old| {
            dir.deleteFile(old) catch continue;
        }

        var frag_iter = newest_frags.valueIterator();
        var first = true;
        const cdb = try cache_root.createFile(cdb_filename, .{});
        defer cdb.close();

        _ = try cdb.write("[");
        while (frag_iter.next()) |info| {
            if (!first) _ = try cdb.write(",\n");
            first = false;

            const fpath = b.pathJoin(&.{ cdb_frags_dirname, info.name });
            const contents = try cache_root.readFile(fpath, file_buf);
            const trimmed = std.mem.trimEnd(u8, contents, ",\n\r\t");
            _ = try cdb.write(trimmed);
        }
        _ = try cdb.write("]");
    }
};

fn addTooling(b: *std.Build, config: struct {
    cdb_gen: *CdbGenerator,
    cli: *std.Build.Step.Compile,
    tests: ?TestArtifacts,
    cppcheck: ?*std.Build.Step.Compile,
    clean_cache: bool,
}) !void {
    const tooling_sources = try collectToolingFiles(b);

    const cdb_step = b.step("cdb", "Generate " ++ CdbGenerator.cdb_filename);
    cdb_step.dependOn(&config.cdb_gen.step);
    b.getInstallStep().dependOn(&config.cdb_gen.step);

    if (findProgram(b, "clang-format")) |clang_format| {
        try addFmtStep(b, .{
            .tooling_sources = tooling_sources,
            .clang_format = clang_format,
        });
    }

    if (config.cppcheck) |cppcheck| {
        const check_step = try addStaticAnalysisStep(b, .{
            .tooling_sources = tooling_sources,
            .cppcheck = cppcheck,
            .cdb_gen = config.cdb_gen,
        });
        check_step.dependOn(&config.cdb_gen.step);
    }

    const cloc: *LOCCounter = .init(b);
    const cloc_step = b.step("cloc", "Count lines of code across the project");
    cloc_step.dependOn(&cloc.step);

    if (config.tests) |tests| {
        try addLLDBStep(b, .{
            .cli = config.cli,
            .tests = tests,
        });
    }

    const clean_step = b.step("clean", "Remove all emitted artifacts");
    const remove_install = b.addRemoveDirTree(b.path(getPrefixRelativePath(b, &.{})));
    clean_step.dependOn(&remove_install.step);
    if (config.clean_cache) {
        const remove_cache = b.addRemoveDirTree(b.path(getCacheRelativePath(b, &.{})));
        clean_step.dependOn(&remove_cache.step);
    }
}

fn addFmtStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    clang_format: []const u8,
}) !void {
    const zig_paths: []const []const u8 = &.{
        "build.zig",
        "build.zig.zon",
        ProjectPaths.test_runner ++ "main.zig",
    };
    const build_fmt = b.addFmt(.{ .paths = zig_paths });
    const build_fmt_check = b.addFmt(.{ .paths = zig_paths, .check = true });

    const fmt = b.addSystemCommand(&.{config.clang_format});
    fmt.addArg("-i");
    fmt.addArgs(config.tooling_sources);
    const fmt_step = b.step("fmt", "Format all project files");
    fmt_step.dependOn(&fmt.step);
    fmt_step.dependOn(&build_fmt.step);

    const fmt_check = b.addSystemCommand(&.{config.clang_format});
    fmt_check.addArgs(&.{ "--dry-run", "--Werror" });
    fmt_check.addArgs(config.tooling_sources);
    const fmt_check_step = b.step("fmt-check", "Check formatting of all project files");
    fmt_check_step.dependOn(&fmt_check.step);
    fmt_check_step.dependOn(&build_fmt_check.step);
}

fn addStaticAnalysisStep(b: *std.Build, config: struct {
    tooling_sources: []const []const u8,
    cppcheck: *std.Build.Step.Compile,
    cdb_gen: *CdbGenerator,
}) !*std.Build.Step {
    const check_step = b.step("check", "Run static analysis on all project files");
    const cppcheck = b.addRunArtifact(config.cppcheck);

    const installed_cppcheck_cache_path = getCacheRelativePath(b, &.{"cppcheck"});
    cppcheck.addArg("--inline-suppr");
    cppcheck.addPrefixedFileArg("--project=", config.cdb_gen.getCdbPath());
    const cppcheck_cache = cppcheck.addPrefixedOutputDirectoryArg(
        "--cppcheck-build-dir=",
        installed_cppcheck_cache_path,
    );
    cppcheck.addArg("--check-level=exhaustive");
    cppcheck.addArgs(&.{ "--error-exitcode=1", "--enable=all" });
    cppcheck.addArgs(&.{
        "-icatch_amalgamated.cpp",
        "--suppress=*:catch_amalgamated.hpp",
        "--suppress=*:magic_enum.hpp",
    });

    const suppressions: []const []const u8 = &.{
        "checkersReport",
        "unmatchedSuppression",
        "missingIncludeSystem",
        "unusedFunction",
    };

    inline for (suppressions) |suppression| {
        cppcheck.addArg("--suppress=" ++ suppression);
    }

    const cppcheck_cache_install = b.addInstallDirectory(.{
        .source_dir = cppcheck_cache,
        .install_dir = .{ .custom = ".." },
        .install_subdir = installed_cppcheck_cache_path,
    });

    cppcheck_cache_install.step.dependOn(&config.cppcheck.step);
    check_step.dependOn(&cppcheck_cache_install.step);
    check_step.dependOn(&cppcheck.step);
    return check_step;
}

fn addLLDBStep(b: *std.Build, config: struct {
    cli: *std.Build.Step.Compile,
    tests: TestArtifacts,
}) !void {
    if (builtin.os.tag == .windows) return;
    const error_msg =
        \\LLDB is is LLVM's CLI debugger whose source code can be found here:
        \\  https://github.com/llvm/llvm-project
        \\
        \\It is available on most major platforms through their corresponding package managers.
    ;

    const dbg = b.step("dbg", "Debug the main executable with lldb");
    const dbg_cli = b.step("dbg-cli", "Debug the cli tests with lldb");
    const dbg_compiler = b.step("dbg-compiler", "Debug the compiler tests with lldb");
    const dbg_core = b.step("dbg-core", "Debug the core tests with lldb");
    const dbg_runner = b.step("dbg-runner", "Debug the runner tests with lldb");
    const steps = [_]struct { *std.Build.Step, *std.Build.Step.Compile }{
        .{ dbg, config.cli },
        .{ dbg_cli, config.tests.cli_tests },
        .{ dbg_compiler, config.tests.compiler_tests },
        .{ dbg_core, config.tests.core_tests },
        .{ dbg_runner, config.tests.runner_tests },
    };

    for (steps) |step| {
        if (findProgram(b, "lldb")) |lldb| {
            var debugger: *std.Build.Step.Run = undefined;

            if (findProgram(b, "coreutils")) |coreutils| {
                debugger = b.addSystemCommand(&.{coreutils});
                debugger.addArgs(&.{ "--coreutils-prog=env", "-i", lldb });
            } else if (findProgram(b, "env")) |env| {
                debugger = b.addSystemCommand(&.{env});
                debugger.addArgs(&.{ "-i", lldb });
            } else {
                debugger = b.addSystemCommand(&.{lldb});
                debugger = b.addSystemCommand(&.{lldb});
            }

            debugger.addArtifactArg(step.@"1");
            step.@"0".dependOn(&debugger.step);
        } else {
            try step.@"0".addError(error_msg, .{});
        }
    }
}

const LOCCounter = struct {
    const LOCResult = struct {
        counts: std.StringHashMap(struct {
            line_count: usize,
            frequency: usize,
        }),
        total_line_count: usize,
        file_count: usize,

        pub fn init(allocator: std.mem.Allocator) LOCResult {
            return .{
                .counts = .init(allocator),
                .total_line_count = 0,
                .file_count = 0,
            };
        }

        // Adds a file to the counts, grouping by un-dotted extension
        pub fn logFile(self: *LOCResult, file_path: []const u8, line_count: usize) !void {
            const ext = std.fs.path.extension(file_path)[1..];
            const gop = try self.counts.getOrPut(ext);

            if (gop.found_existing) {
                gop.value_ptr.line_count += line_count;
                gop.value_ptr.frequency += 1;
            } else {
                gop.value_ptr.* = .{
                    .line_count = line_count,
                    .frequency = 1,
                };
            }
            self.file_count += 1;
            self.total_line_count += line_count;
        }

        pub fn print(self: *const LOCResult) !void {
            const stdout_handle = std.fs.File.stdout();
            var stdout_buf: [1024]u8 = undefined;
            var stdout_writer = stdout_handle.writer(&stdout_buf);
            const stdout = &stdout_writer.interface;

            try stdout.print("Scanned {d} total files:\n", .{self.file_count});

            var count_iter = self.counts.iterator();
            while (count_iter.next()) |entry| {
                try stdout.print("  {d} total {s} files: {d} LOC\n", .{
                    entry.value_ptr.frequency,
                    entry.key_ptr.*,
                    entry.value_ptr.line_count,
                });
            }
            try stdout.print("Total: {d} LOC\n", .{self.total_line_count});

            try stdout.flush();
        }
    };

    step: std.Build.Step,

    pub fn init(
        b: *std.Build,
    ) *LOCCounter {
        const self = b.allocator.create(LOCCounter) catch @panic("OOM");
        self.* = .{
            .step = .init(.{
                .id = .custom,
                .name = "cloc",
                .owner = b,
                .makeFn = count,
            }),
        };
        return self;
    }

    fn count(step: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
        const b = step.owner;

        const extensions = [_][]const u8{ ".cpp", ".hpp", ".zig", ".conch" };
        var files: std.ArrayList([]const u8) = .empty;

        try files.appendSlice(
            b.allocator,
            try collectFiles(b, "packages", .{
                .allowed_extensions = &extensions,
                .extra_files = &.{"build.zig"},
            }),
        );

        try files.appendSlice(
            b.allocator,
            try collectFiles(b, "apps", .{ .allowed_extensions = &extensions }),
        );

        const build_dir = b.build_root.handle;
        const buffer = try b.allocator.alloc(u8, 100 * 1024);
        var result: LOCResult = .init(b.allocator);

        for (files.items) |file| {
            const contents = try build_dir.readFile(file, buffer);
            var it = std.mem.tokenizeAny(u8, contents, "\r\n");

            var lines: usize = 0;
            while (it.next()) |line| {
                const trimmed = std.mem.trim(u8, line, " \t");
                if (trimmed.len > 0 and !std.mem.startsWith(u8, trimmed, "//")) {
                    lines += 1;
                }
            }

            try result.logFile(file, lines);
        }

        try result.print();
    }
};

const target_queries = [_]std.Target.Query{
    .{ .cpu_arch = .x86_64, .os_tag = .macos },
    .{ .cpu_arch = .aarch64, .os_tag = .macos },

    .{ .cpu_arch = .x86, .os_tag = .linux },
    .{ .cpu_arch = .x86_64, .os_tag = .linux },
    .{ .cpu_arch = .aarch64, .os_tag = .linux },
    .{ .cpu_arch = .powerpc, .os_tag = .linux },
    .{ .cpu_arch = .powerpc64, .os_tag = .linux },
    .{ .cpu_arch = .powerpc64le, .os_tag = .linux },
    .{ .cpu_arch = .riscv32, .os_tag = .linux },
    .{ .cpu_arch = .riscv64, .os_tag = .linux },
    .{ .cpu_arch = .loongarch64, .os_tag = .linux },

    .{ .cpu_arch = .x86_64, .os_tag = .freebsd },
    .{ .cpu_arch = .aarch64, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc64, .os_tag = .freebsd },
    .{ .cpu_arch = .powerpc64le, .os_tag = .freebsd },
    .{ .cpu_arch = .riscv64, .os_tag = .freebsd },

    .{ .cpu_arch = .x86, .os_tag = .netbsd },
    .{ .cpu_arch = .x86_64, .os_tag = .netbsd },
    .{ .cpu_arch = .aarch64, .os_tag = .netbsd },

    .{ .cpu_arch = .x86, .os_tag = .windows },
    .{ .cpu_arch = .x86_64, .os_tag = .windows },
    .{ .cpu_arch = .aarch64, .os_tag = .windows },
};

fn addPackageStep(b: *std.Build, config: struct {
    compile_only: bool,
    cxx_flags: []const []const u8,
}) !void {
    // TODO, support packaging correctly with LLVM
    const package_step = b.step("package", "Build the artifacts for packaging");
    if (true) {
        return package_step.addError("Packing is currently unsupported as LLVM integration is a WIP", .{});
    }

    const uncompressed_package_dir: []const []const u8 = &.{ "package", "uncompressed" };
    const compressed_package_dir: []const []const u8 = &.{ "package", "compressed" };

    var zon_buf: [2 * 1024]u8 = undefined;
    const raw_zon_contents = try b.build_root.handle.readFile("build.zig.zon", &zon_buf);
    zon_buf[raw_zon_contents.len] = 0;
    const zon_contents = zon_buf[0..raw_zon_contents.len :0];

    const parsed = try std.zon.parse.fromSlice(
        struct { version: []const u8 },
        b.allocator,
        zon_contents,
        null,
        .{ .ignore_unknown_fields = true },
    );
    const version = parsed.version;

    for (target_queries) |query| {
        const resolved_target = b.resolveTargetQuery(query);
        const artifacts = try addArtifacts(b, .{
            .target = resolved_target,
            .optimize = .ReleaseFast,
            .cxx_flags = config.cxx_flags,
            .cdb_steps = null,
            .skip_tests = true,
            .skip_cppcheck = true,
            .behavior = .standalone,
            .auto_install = false,
        });

        std.debug.assert(artifacts.tests == null);

        try configurePackArtifacts(b, .{
            .artifacts = &.{ artifacts.libcompiler, artifacts.libcli, artifacts.cli },
            .target = resolved_target,
            .version = version,
        });

        const package_artifact_dirname = try std.fmt.allocPrint(
            b.allocator,
            "conch-{s}-{s}",
            .{ try query.zigTriple(b.allocator), version },
        );

        const package_artifact_dir_path = b.pathJoin(uncompressed_package_dir ++ .{package_artifact_dirname});
        const platform = b.addInstallArtifact(
            artifacts.cli,
            .{
                .dest_dir = .{
                    .override = .{ .custom = package_artifact_dir_path },
                },
            },
        );
        package_step.dependOn(&platform.step);

        if (!config.compile_only) {
            const tar = findProgram(b, "tar");
            const zip = findProgram(b, "zip");
            if (zip == null or tar == null) {
                return package_step.addError(
                    \\Packaging cannot run without zip and tar commands
                    \\  zip: {s}
                    \\  tar: {s}
                , .{ zip orelse "null", tar orelse "null" });
            }

            const legal_paths = [_]struct { std.Build.LazyPath, []const u8 }{
                .{ b.path("LICENSE"), "LICENSE" },
                .{ b.path("README.md"), "README.md" },
                .{ b.path(".github/CHANGELOG.md"), "CHANGELOG.md" },
            };

            var file_installs: [legal_paths.len]*std.Build.Step = undefined;
            for (legal_paths, 0..) |path, i| {
                const install_file_step = b.addInstallFileWithDir(
                    path.@"0",
                    .{ .custom = package_artifact_dir_path },
                    path.@"1",
                );
                package_step.dependOn(&install_file_step.step);
                file_installs[i] = &install_file_step.step;
            }

            // Zip is only needed on windows
            if (query.os_tag.? == .windows) {
                const zip_filename = try std.fmt.allocPrint(
                    b.allocator,
                    "{s}.zip",
                    .{package_artifact_dirname},
                );

                const zipper = b.addSystemCommand(&.{ zip.?, "-r" });
                const output_zip = zipper.addOutputFileArg(zip_filename);
                zipper.addArg(package_artifact_dirname);
                zipper.setCwd(b.path(getPrefixRelativePath(b, uncompressed_package_dir)));
                _ = zipper.captureStdErr();

                zipper.step.dependOn(&platform.step);
                package_step.dependOn(&zipper.step);
                for (file_installs) |step| {
                    zipper.step.dependOn(step);
                }

                const copy_zip = b.addInstallFileWithDir(
                    output_zip,
                    .{ .custom = b.pathJoin(compressed_package_dir) },
                    zip_filename,
                );
                package_step.dependOn(&copy_zip.step);
            }

            // All platforms get an archive because I'm nice
            const tar_filename = try std.fmt.allocPrint(
                b.allocator,
                "{s}.tar.gz",
                .{package_artifact_dirname},
            );

            const archiver = b.addSystemCommand(&.{ tar.?, "-czf" });
            const output_tar = archiver.addOutputFileArg(tar_filename);
            archiver.addArg("-C");
            archiver.addArg(getPrefixRelativePath(b, uncompressed_package_dir));
            archiver.addArg(package_artifact_dirname);
            _ = archiver.captureStdErr();

            archiver.step.dependOn(&platform.step);
            package_step.dependOn(&archiver.step);
            for (file_installs) |step| {
                archiver.step.dependOn(step);
            }

            const copy_tar = b.addInstallFileWithDir(
                output_tar,
                .{ .custom = b.pathJoin(compressed_package_dir) },
                tar_filename,
            );
            package_step.dependOn(&copy_tar.step);
        }
    }
}

fn configurePackArtifacts(b: *std.Build, config: struct {
    artifacts: []const *std.Build.Step.Compile,
    target: std.Build.ResolvedTarget,
    version: []const u8,
}) !void {
    for (config.artifacts) |artifact| {
        artifact.root_module.strip = true;
        artifact.out_filename = blk: {
            if (config.target.result.os.tag == .windows) {
                break :blk try std.fmt.allocPrint(
                    b.allocator,
                    "{s}-{s}.exe",
                    .{ artifact.name, config.version },
                );
            } else {
                break :blk try std.fmt.allocPrint(
                    b.allocator,
                    "{s}-{s}",
                    .{ artifact.name, config.version },
                );
            }
        };
    }
}

fn collectFiles(
    b: *std.Build,
    directory: []const u8,
    options: struct {
        allowed_extensions: []const []const u8 = &.{".cpp"},
        dropped_files: ?[]const [:0]const u8 = null,
        extra_files: ?[]const []const u8 = null,
    },
) ![]const []const u8 {
    var dir = try b.build_root.handle.openDir(directory, .{ .iterate = true });
    defer dir.close();

    var walker = try dir.walk(b.allocator);
    defer walker.deinit();

    var paths: std.ArrayList([]const u8) = .empty;
    collector: while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        for (options.allowed_extensions) |ext| {
            if (std.mem.endsWith(u8, entry.basename, ext)) break;
        } else continue :collector;

        if (options.dropped_files) |drop| for (drop) |drop_file| {
            if (std.mem.eql(u8, drop_file, entry.basename)) continue :collector;
        };

        const full_path = b.pathJoin(&.{ directory, entry.path });
        try paths.append(b.allocator, full_path);
    }

    if (options.extra_files) |extra_files| {
        try paths.appendSlice(b.allocator, extra_files);
    }
    return paths.items;
}

fn collectToolingFiles(b: *std.Build) ![]const []const u8 {
    return std.mem.concat(b.allocator, []const u8, &.{
        try ProjectPaths.compiler.files(b),
        try ProjectPaths.cli.files(b),
        try ProjectPaths.core.files(b),
        try collectFiles(b, ProjectPaths.test_runner, .{ .allowed_extensions = &.{".cpp"} }),
    });
}

/// Resolves the relative path with its root at the cache directory
fn getCacheRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    return b.cache_root.join(b.allocator, paths) catch @panic("OOM");
}

/// Resolves the relative path with its root at the installation directory
fn getPrefixRelativePath(b: *std.Build, paths: []const []const u8) []const u8 {
    return b.pathJoin(std.mem.concat(
        b.allocator,
        []const u8,
        &.{ &.{std.fs.path.basename(b.install_prefix)}, paths },
    ) catch @panic("OOM"));
}

fn findProgram(b: *std.Build, cmd: []const u8) ?[]const u8 {
    return b.findProgram(&.{cmd}, &.{}) catch null;
}
