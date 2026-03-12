//! https://github.com/allyourcodebase/kcov/blob/master/build.zig
const std = @import("std");

const KcovBuilder = @import("../KcovBuilder.zig");

const version: std.SemanticVersion = .{
    .major = 43,
    .minor = 0,
    .patch = 0,
};
const version_str = std.fmt.comptimePrint("{d}.{d}", .{ version.major, version.major });

pub const BinToCSourceFiles = struct {
    library_cc: std.Build.LazyPath,
    bash_redirector_library_cc: std.Build.LazyPath,
    bash_cloexec_library_cc: std.Build.LazyPath,
    python_helper_cc: std.Build.LazyPath,
    bash_helper_cc: std.Build.LazyPath,
    kcov_system_library_cc: std.Build.LazyPath,
    html_data_files_cc: std.Build.LazyPath,
    version_c: std.Build.LazyPath,
};

const mib = 1024 * 1024;

pub fn runBinToCSource(builder: *const KcovBuilder) BinToCSourceFiles {
    const b = builder.b;
    const mod = b.createModule(.{
        .root_source_file = b.path("packages/third-party/kcov/utils/bin_to_c_source.zig"),
        .target = b.graph.host,
        .optimize = .ReleaseFast,
    });

    const bin_to_c_source = b.addExecutable(.{
        .name = "bin_to_c_source",
        .root_module = mod,
    });

    const library_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.addArtifactArg(builder.kcov_sowrapper);
        run_bin_to_c_source.addArg("__library");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "library.cc");
    };

    const bash_redirector_library_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.addArtifactArg(builder.bash_execve_redirector);
        run_bin_to_c_source.addArg("bash_redirector_library");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "bash-redirector-library.cc");
    };

    const bash_cloexec_library_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.addArtifactArg(builder.bash_tracefd_cloexec);
        run_bin_to_c_source.addArg("bash_cloexec_library");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "bash-cloexec-library.cc");
    };

    const kcov_system_library_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.max_stdio_size = 256 * mib;
        run_bin_to_c_source.addArtifactArg(builder.kcov_system_lib);
        run_bin_to_c_source.addArg("kcov_system_library");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "kcov-system-library.cc");
    };

    const python_helper_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.addFileArg(builder.metadata.root.path(b, "src/engines/python-helper.py"));
        run_bin_to_c_source.addArg("python_helper");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "python-helper.cc");
    };

    const bash_helper_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.addFileArg(builder.metadata.root.path(b, "src/engines/bash-helper.sh"));
        run_bin_to_c_source.addArg("bash_helper");
        run_bin_to_c_source.addFileArg(builder.metadata.root.path(b, "src/engines/bash-helper-debug-trap.sh"));
        run_bin_to_c_source.addArg("bash_helper_debug_trap");
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "bash-helper.cc");
    };

    const html_data_files_cc = blk: {
        const run_bin_to_c_source = b.addRunArtifact(bin_to_c_source);
        run_bin_to_c_source.clearEnvironment();
        run_bin_to_c_source.max_stdio_size = 32 * mib;
        for (
            [_][]const u8{
                "data/bcov.css",
                "data/amber.png",
                "data/glass.png",
                "data/source-file.html",
                "data/index.html",
                "data/js/handlebars.js",
                "data/js/kcov.js",
                "data/js/jquery.min.js",
                "data/js/jquery.tablesorter.min.js",
                "data/js/jquery.tablesorter.widgets.min.js",
                "data/tablesorter-theme.css",
            },
            [_][]const u8{
                "css_text",
                "icon_amber",
                "icon_glass",
                "source_file_text",
                "index_text",
                "handlebars_text",
                "kcov_text",
                "jquery_text",
                "tablesorter_text",
                "tablesorter_widgets_text",
                "tablesorter_theme_text",
            },
        ) |path, name| {
            run_bin_to_c_source.addFileArg(builder.metadata.root.path(b, path));
            run_bin_to_c_source.addArg(name);
        }
        break :blk renameLazyPath(b, run_bin_to_c_source.captureStdOut(), "html-data-files.cc");
    };

    const version_c = blk: {
        const write_files = b.addWriteFiles();
        break :blk write_files.add("version.c", "const char *kcov_version = \"" ++ version_str ++ "\";");
    };

    return .{
        .library_cc = library_cc,
        .bash_redirector_library_cc = bash_redirector_library_cc,
        .bash_cloexec_library_cc = bash_cloexec_library_cc,
        .python_helper_cc = python_helper_cc,
        .bash_helper_cc = bash_helper_cc,
        .kcov_system_library_cc = kcov_system_library_cc,
        .html_data_files_cc = html_data_files_cc,
        .version_c = version_c,
    };
}

fn renameLazyPath(b: *std.Build, lazy_path: std.Build.LazyPath, basename: []const u8) std.Build.LazyPath {
    const write_files = b.addWriteFiles();
    return write_files.addCopyFile(lazy_path, basename);
}

pub const sources = [_][]const u8{
    "capabilities.cc",
    "collector.cc",
    "configuration.cc",
    "engine-factory.cc",
    "engines/bash-engine.cc",
    "engines/system-mode-engine.cc",
    "engines/system-mode-file-format.cc",
    "engines/python-engine.cc",
    "filter.cc",
    "main.cc",
    "merge-file-parser.cc",
    "output-handler.cc",
    "parser-manager.cc",
    "reporter.cc",
    "source-file-cache.cc",
    "utils.cc",
    "writers/cobertura-writer.cc",
    "writers/codecov-writer.cc",
    "writers/json-writer.cc",
    "writers/html-writer.cc",
    "writers/sonarqube-xml-writer.cc",
    "writers/writer-base.cc",
    "system-mode/file-data.cc",
};
