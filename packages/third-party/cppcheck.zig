const std = @import("std");

const Dependency = @import("Dependency.zig");

/// Compiles cppcheck from source using the flags given by:
/// https://github.com/danmar/cppcheck#g-for-experts
pub fn build(b: *std.Build) !Dependency {
    const upstream = b.dependency("cppcheck", .{});
    const includes: []const std.Build.LazyPath = &.{
        upstream.path("externals"),
        upstream.path("externals/simplecpp"),
        upstream.path("externals/tinyxml2"),
        upstream.path("externals/picojson"),
        upstream.path("lib"),
        upstream.path("frontend"),
    };

    const root = upstream.path(".");
    const target = b.graph.host;

    // The path needs to be fixed on windows due to cppcheck internals
    const cfg_path = blk: {
        const raw_cfg_path = try root.getPath3(b, null).toString(b.allocator);
        if (target.result.os.tag == .windows) {
            break :blk try std.mem.replaceOwned(u8, b.allocator, raw_cfg_path, "\\", "/");
        }
        break :blk raw_cfg_path;
    };

    const files_dir = try std.fmt.allocPrint(
        b.allocator,
        "-DFILESDIR=\"{s}\"",
        .{cfg_path},
    );

    const mod = b.createModule(.{
        .target = target,
        .optimize = .ReleaseFast,
        .link_libcpp = true,
    });

    for (includes) |include| {
        mod.addIncludePath(include);
    }

    mod.addCSourceFiles(.{
        .root = root,
        .files = &sources,
        .flags = &.{ files_dir, "-Uunix", "-std=c++11" },
    });

    return .{
        .upstream = upstream,
        .artifact = b.addExecutable(.{
            .name = "cppcheck",
            .root_module = mod,
        }),
    };
}

const sources = [_][]const u8{
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
