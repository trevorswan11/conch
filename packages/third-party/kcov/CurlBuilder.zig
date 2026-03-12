const std = @import("std");

const Dependency = @import("../Dependency.zig");
const Config = Dependency.Config;
const Artifact = Dependency.Artifact;

const curl = @import("sources/curl.zig");
const mbedtls = @import("sources/mbedtls.zig");

const zlib = @import("../zlib.zig");
const zstd = @import("../zstd.zig");

const c_flags: []const []const u8 = &.{"-fvisibility=hidden"};

const Self = @This();

const Metadata = struct {
    upstream: *std.Build.Dependency,
    config: Config,
    include_root: std.Build.LazyPath,
    lib_root: std.Build.LazyPath,
    src_root: std.Build.LazyPath,
};

b: *std.Build,
metadata: Metadata,

config_header: *std.Build.Step.ConfigHeader = undefined,
libmbedtls: Artifact = undefined,
libcurl: Artifact = undefined,
execurl: Artifact = undefined,

/// Compiles curl from source (lib and exe).
/// https://github.com/allyourcodebase/curl
pub fn build(b: *std.Build, config: Config) ?*Self {
    const upstream = b.lazyDependency("curl", .{});
    const libmbedtls = buildMbedtls(b, config);
    if (upstream == null or libmbedtls == null) return null;

    const self = b.allocator.create(Self) catch @panic("OOM");
    self.* = .{
        .b = b,
        .metadata = .{
            .upstream = upstream.?,
            .config = config,
            .include_root = upstream.?.path("include"),
            .lib_root = upstream.?.path("lib"),
            .src_root = upstream.?.path("src"),
        },
        .libmbedtls = libmbedtls.?,
    };

    self.config_header = self.makeConfigHeader();
    self.libcurl = self.buildCurlLib();
    self.execurl = self.buildCurlExe();
    return self;
}

/// Compiles mbedtls from source as a static library.
/// https://github.com/allyourcodebase/mbedtls
pub fn buildMbedtls(b: *std.Build, config: Config) ?Artifact {
    const upstream_dep = b.lazyDependency("mbedtls", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    if (upstream_dep) |upstream| {
        mod.addIncludePath(upstream.path("include"));
        mod.addCSourceFiles(.{
            .root = upstream.path("library"),
            .files = &mbedtls.sources,
        });

        if (target.result.os.tag == .freebsd) {
            mod.addCMacro("__BSD_VISIBLE", "1");
        }

        mod.addCMacro("MBEDTLS_ENTROPY_C", "");
        mod.addCMacro("MBEDTLS_CTR_DRBG_C", "");
        mod.addCMacro("MBEDTLS_PLATFORM_C", "");

        if (target.result.os.tag == .windows) {
            mod.linkSystemLibrary("bcrypt", .{});
        } else {
            mod.addCMacro("MBEDTLS_PLATFORM_ENTROPY", "");
            mod.addCMacro("MBEDTLS_HAVE_TIME", "");
        }

        const lib = b.addLibrary(.{
            .name = "mbedtls",
            .root_module = mod,
        });
        lib.installHeadersDirectory(upstream.path("include/mbedtls"), "mbedtls", .{});
        lib.installHeadersDirectory(upstream.path("include/psa"), "psa", .{});
        return lib;
    } else return null;
}

/// Handles CA and the resulting configuration
fn makeConfigHeader(self: *const Self) *std.Build.Step.ConfigHeader {
    const b = self.b;
    const target = self.metadata.config.target;

    const ca_bundle_autodetect = target.query.isNative() and target.result.os.tag != .windows;
    var ca_bundle: []const u8 = "auto";
    const ca_path_autodetect = target.query.isNative() and target.result.os.tag != .windows;
    var ca_path: []const u8 = "auto";

    if (ca_bundle_autodetect) {
        const search_paths = [_][]const u8{
            "/etc/ssl/certs/ca-certificates.crt",
            "/etc/pki/tls/certs/ca-bundle.crt",
            "/usr/share/ssl/certs/ca-bundle.crt",
            "/usr/local/share/certs/ca-root-nss.crt",
            "/etc/ssl/cert.pem",
        };
        for (search_paths) |search_path| {
            std.fs.accessAbsolute(search_path, .{}) catch continue;
            ca_bundle = search_path;
            break;
        }
    }

    if (ca_path_autodetect) blk: {
        const search_ca_path = "/etc/ssl/certs";
        var ca_dir = std.fs.openDirAbsolute(search_ca_path, .{ .iterate = true }) catch break :blk;
        defer ca_dir.close();

        var walker = ca_dir.walk(b.allocator) catch @panic("OOM");
        defer walker.deinit();
        while (walker.next() catch break :blk) |entry| {
            if (entry.basename.len != 10) continue;
            if (!std.mem.endsWith(u8, entry.basename, ".0")) continue;
            ca_path = search_ca_path;
            break;
        }
    }

    return curl.configHeader(
        b,
        .{ .cmake = self.metadata.lib_root.path(b, "curl_config-cmake.h.in") },
        target,
        ca_bundle,
        ca_path,
    );
}

fn buildCurlLib(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });
    addFrameworkSearchPaths(mod, target);

    mod.addCMacro("BUILDING_LIBCURL", "1");
    mod.addCMacro("CURL_STATICLIB", "1");
    mod.addCMacro("CURL_HIDDEN_SYMBOLS", "1");
    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addIncludePath(self.metadata.include_root);
    mod.addIncludePath(self.metadata.lib_root);
    mod.addCSourceFiles(.{
        .root = self.metadata.lib_root,
        .files = curl.sources,
        .flags = c_flags,
    });

    if (target.result.os.tag == .linux) {
        mod.addCMacro("_GNU_SOURCE", "1");
    }

    mod.addCMacro("HAVE_PTHREAD_H", "1");
    mod.linkSystemLibrary("pthread", .{});

    if (target.result.os.tag.isDarwin()) {
        mod.linkFramework("CoreFoundation", .{});
        mod.linkFramework("CoreServices", .{});
        mod.linkFramework("SystemConfiguration", .{});
    }

    if (target.result.os.tag == .windows) {
        mod.linkSystemLibrary("ws2_32", .{});
        mod.linkSystemLibrary("iphlpapi", .{});
        mod.linkSystemLibrary("bcrypt", .{});
    }

    mod.linkLibrary(self.libmbedtls);
    mod.addCMacro("MBEDTLS_VERSION", mbedtls.version_str);

    const zlib_dep = zlib.build(b, self.metadata.config);
    mod.linkLibrary(zlib_dep.artifact);
    const zstd_dep = zstd.build(b, self.metadata.config);
    mod.linkLibrary(zstd_dep.artifact);
    mod.addConfigHeader(self.config_header);

    const lib = b.addLibrary(.{
        .name = "curl",
        .root_module = mod,
    });
    lib.installHeadersDirectory(self.metadata.include_root, "", .{});
    return lib;
}

fn buildCurlExe(self: *const Self) Artifact {
    const b = self.b;
    const target = self.metadata.config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = self.metadata.config.optimize,
        .link_libc = true,
    });
    addFrameworkSearchPaths(mod, target);

    mod.addCMacro("HAVE_CONFIG_H", "1");
    mod.addCMacro("CURL_STATICLIB", "1");
    mod.addIncludePath(self.metadata.include_root);
    mod.addIncludePath(self.metadata.lib_root);
    mod.addIncludePath(self.metadata.src_root);
    mod.addCSourceFiles(.{
        .root = self.metadata.src_root,
        .files = curl.exe_sources,
        .flags = c_flags,
    });

    mod.addConfigHeader(self.config_header);
    const exe = b.addExecutable(.{
        .name = "curl",
        .root_module = mod,
    });
    mod.linkLibrary(self.libcurl);
    return exe;
}

pub fn addFrameworkSearchPaths(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .macos) return;
    const b = mod.owner;
    if (b.graph.env_map.get("SDKROOT")) |sdkroot| {
        mod.addFrameworkPath(.{ .cwd_relative = b.fmt("{s}/System/Library/Frameworks", .{sdkroot}) });
        mod.addSystemIncludePath(.{ .cwd_relative = b.fmt("{s}/usr/include", .{sdkroot}) });
    }
}
