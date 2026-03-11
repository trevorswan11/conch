const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const curl = @import("sources/curl.zig");

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");
const mbedtls = @import("mbedtls.zig");

const version: std.SemanticVersion = .{
    .major = 8,
    .minor = 18,
    .patch = 0,
};
pub const version_str = std.fmt.comptimePrint("{f}", .{version});

const c_flags: []const []const u8 = &.{"-fvisibility=hidden"};

const Self = @This();

upstream: *std.Build.Dependency,
lib: Artifact,
exe: Artifact,

/// Compiles curl from source (lib and exe).
/// https://github.com/allyourcodebase/curl
pub fn build(b: *std.Build, config: Config) ?Self {
    const upstream_dep = b.lazyDependency("curl", .{});
    const mbedtls_dep = mbedtls.build(b, config);
    if (upstream_dep == null or mbedtls_dep == null) return null;
    const upstream = upstream_dep.?;

    const target = config.target;
    const lib_mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });
    addFrameworkSearchPaths(lib_mod, target);

    const exe_mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });
    addFrameworkSearchPaths(exe_mod, target);

    const include_root = upstream.path("include");
    const lib_root = upstream.path("lib");
    const src_root = upstream.path("src");

    lib_mod.addCMacro("BUILDING_LIBCURL", "1");
    lib_mod.addCMacro("CURL_STATICLIB", "1");
    lib_mod.addCMacro("CURL_HIDDEN_SYMBOLS", "1");
    lib_mod.addCMacro("HAVE_CONFIG_H", "1");
    lib_mod.addIncludePath(include_root);
    lib_mod.addIncludePath(lib_root);
    lib_mod.addCSourceFiles(.{
        .root = lib_root,
        .files = curl.sources,
        .flags = c_flags,
    });

    exe_mod.addCMacro("HAVE_CONFIG_H", "1");
    exe_mod.addCMacro("CURL_STATICLIB", "1");
    exe_mod.addIncludePath(include_root);
    exe_mod.addIncludePath(lib_root);
    exe_mod.addIncludePath(src_root);
    exe_mod.addCSourceFiles(.{
        .root = src_root,
        .files = curl.exe_sources,
        .flags = c_flags,
    });

    if (target.result.os.tag == .linux) {
        lib_mod.addCMacro("_GNU_SOURCE", "1");
    }

    lib_mod.addCMacro("HAVE_PTHREAD_H", "1");
    lib_mod.linkSystemLibrary("pthread", .{});

    if (target.result.os.tag.isDarwin()) {
        lib_mod.linkFramework("CoreFoundation", .{});
        lib_mod.linkFramework("CoreServices", .{});
        lib_mod.linkFramework("SystemConfiguration", .{});
    }

    if (target.result.os.tag == .windows) {
        lib_mod.linkSystemLibrary("ws2_32", .{});
        lib_mod.linkSystemLibrary("iphlpapi", .{});
        lib_mod.linkSystemLibrary("bcrypt", .{});
    }

    lib_mod.linkLibrary(mbedtls_dep.?.artifact);
    lib_mod.addCMacro("MBEDTLS_VERSION", mbedtls.version_str);

    const zlib_dep = zlib.build(b, config);
    lib_mod.linkLibrary(zlib_dep.artifact);
    const zstd_dep = zstd.build(b, config);
    lib_mod.linkLibrary(zstd_dep.artifact);

    // CA handling
    var ca_bundle: []const u8 = "auto";
    var ca_path: []const u8 = "auto";
    const ca_embed: ?[]const u8 = null;

    const ca_bundle_autodetect = std.mem.eql(u8, ca_bundle, "auto") and target.query.isNative() and target.result.os.tag != .windows;
    var ca_bundle_set = false;

    const ca_path_autodetect = std.mem.eql(u8, ca_path, "auto") and target.query.isNative() and target.result.os.tag != .windows;
    var ca_path_set = false;

    if (ca_bundle_autodetect or ca_path_autodetect) {
        if (ca_bundle_autodetect) {
            for ([_][]const u8{
                "/etc/ssl/certs/ca-certificates.crt",
                "/etc/pki/tls/certs/ca-bundle.crt",
                "/usr/share/ssl/certs/ca-bundle.crt",
                "/usr/local/share/certs/ca-root-nss.crt",
                "/etc/ssl/cert.pem",
            }) |search_ca_bundle_path| {
                std.fs.accessAbsolute(search_ca_bundle_path, .{}) catch continue;
                ca_bundle = search_ca_bundle_path;
                ca_bundle_set = true;
                break;
            }
        }

        if (ca_path_autodetect and !ca_path_set) {
            const search_ca_path: []const u8 = "/etc/ssl/certs";
            var ca_dir = std.fs.openDirAbsolute(search_ca_path, .{ .iterate = true }) catch @panic("dir open failed");
            defer ca_dir.close();

            var ca_dir_it = ca_dir.iterate();
            while (ca_dir_it.next() catch @panic("dir iter failed")) |item| {
                if (item.name.len != 10) continue;
                if (!std.mem.endsWith(u8, item.name, ".0")) continue;
                ca_path = search_ca_path;
                ca_path_set = true;
                break;
            }
        }

        var ca_embed_set = false;
        if (ca_embed) |embed_path| {
            if (std.fs.accessAbsolute(embed_path, .{})) |_| {
                ca_embed_set = true;
            } else |err| {
                std.debug.panic("CA bundle to embed is missing: {s} ({})", .{ embed_path, err });
            }
        }
    }

    const curl_config = curl.configHeader(b, .{ .cmake = lib_root.path(b, "curl_config-cmake.h.in") }, target);
    lib_mod.addConfigHeader(curl_config);
    exe_mod.addConfigHeader(curl_config);

    const lib = b.addLibrary(.{
        .name = "curl",
        .root_module = lib_mod,
    });
    lib.installHeadersDirectory(include_root, ".", .{});

    const exe = b.addExecutable(.{
        .name = "curl",
        .root_module = exe_mod,
    });
    exe_mod.linkLibrary(lib);

    return .{
        .upstream = upstream,
        .lib = lib,
        .exe = exe,
    };
}

pub fn addFrameworkSearchPaths(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .macos) return;
    const b = mod.owner;
    if (b.graph.env_map.get("SDKROOT")) |sdkroot| {
        mod.addFrameworkPath(.{ .cwd_relative = b.fmt("{s}/System/Library/Frameworks", .{sdkroot}) });
        mod.addSystemIncludePath(.{ .cwd_relative = b.fmt("{s}/usr/include", .{sdkroot}) });
    }
}
