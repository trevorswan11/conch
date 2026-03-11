const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

const zlib = @import("zlib.zig");
const zstd = @import("zstd.zig");
const mbedtls = @import("mbedtls.zig");

const version: std.SemanticVersion = .{
    .major = 8,
    .minor = 18,
    .patch = 0,
};

const Self = @This();

upstream: *std.Build.Dependency,
lib: *std.Build.Step.Compile,
exe: *std.Build.Step.Compile,

/// Compiles curl from source under ReleaseFast.
/// https://github.com/allyourcodebase/curl
pub fn build(b: *std.Build, config: Config) !Self {
    const upstream = b.dependency("curl", .{});
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

    const use_schannel = false;
    const use_mbedtls = true;
    const use_wolfssl = false;
    const use_gnutls = false;
    const use_rustls = false;
    const use_openssl = false;
    const default_ssl_backend: ?bool = null;

    // Dependencies
    const use_brotli = false;
    const use_gsasl = false;
    const use_libpsl = false;
    const use_libssh2 = false;
    const use_libssh = false;
    const use_libuv = false;
    const use_zlib = true;
    const use_zstd = true;
    const enable_ares = false;
    const use_apple_idn = false;
    const use_libidn2 = false;
    const use_librtmp = false;
    const use_nghttp2 = false;
    const use_ngtcp2 = false;
    const use_quiche = false;
    const use_win32_idn = false;
    const use_win32_ldap = true;

    // Enabling features

    const enable_windows_sspi = false;
    const enable_ipv6 = true;
    const enable_threaded_resolver = true;
    const enable_unix_sockets = true;
    const ech = false;
    const httpsrr = false;
    const use_openssl_quic = false;
    const disable_openssl_auto_load_config = false;

    // Disabling features
    const disable_altsvc = false;
    const disable_cookies = false;
    const disable_basic_auth = false;
    const disable_bearer_auth = false;
    const disable_digest_auth = false;
    const disable_kerberos_auth = false;
    const disable_negotiate_auth = false;
    const disable_aws = false;
    const disable_dict = false;
    const disable_doh = false;
    const disable_file = false;
    const disable_ftp = false;
    const disable_getoptions = false;
    const disable_gopher = false;
    const disable_headers_api = false;
    const disable_hsts = false;
    const disable_http = false;
    const disable_http_auth = false;
    const disable_imap = false;
    const disable_ldap = true;
    const disable_ldaps = true;
    const disable_libcurl_option = false;
    const disable_mime = false;
    const disable_form_api = false;
    const disable_mqtt = false;
    const disable_bindlocal = false;
    const disable_netrc = false;
    const disable_ntlm = false;
    const disable_parsedate = false;
    const disable_pop3 = false;
    const disable_progress_meter = false;
    const disable_proxy = false;
    const disable_ipfs = false;
    const disable_rtsp = false;
    const disable_sha512_256 = false;
    const disable_shuffle_dns = false;
    const disable_smb = false;
    const disable_smtp = false;
    const disable_socketpair = false;
    const disable_websockets = false;
    const disable_telnet = false;
    const disable_tftp = false;
    const disable_verbose_strings = false;

    // CA bundle options
    var ca_bundle: []const u8 = "auto";
    const ca_fallback = false;
    var ca_path: []const u8 = "auto";
    const ca_embed: ?[]const u8 = null;

    const disable_ca_search = false;
    const ca_search_safe = false;

    const use_ssls_export = false;

    const hidden_symbols = true;

    const c_flags: []const []const u8 = if (hidden_symbols) &.{"-fvisibility=hidden"} else &.{};

    lib_mod.addCMacro("BUILDING_LIBCURL", "1");
    lib_mod.addCMacro("CURL_STATICLIB", "1");
    lib_mod.addCMacro("CURL_HIDDEN_SYMBOLS", "1");
    lib_mod.addCMacro("HAVE_CONFIG_H", "1");
    lib_mod.addIncludePath(include_root);
    lib_mod.addIncludePath(lib_root);
    lib_mod.addCSourceFiles(.{
        .root = lib_root,
        .files = sources,
        .flags = c_flags,
    });

    exe_mod.addCMacro("HAVE_CONFIG_H", "1");
    exe_mod.addCMacro("CURL_STATICLIB", "1");
    exe_mod.addIncludePath(include_root);
    exe_mod.addIncludePath(lib_root);
    exe_mod.addIncludePath(src_root);
    exe_mod.addCSourceFiles(.{
        .root = src_root,
        .files = exe_sources,
        .flags = c_flags,
    });

    if (target.result.os.tag == .linux) {
        // // Required for accept4(), pipe2(), sendmmsg()
        lib_mod.addCMacro("_GNU_SOURCE", "1");
    }

    lib_mod.addCMacro("HAVE_PTHREAD_H", "1");
    lib_mod.linkSystemLibrary("pthread", .{});

    if (target.result.os.tag.isDarwin()) {
        lib_mod.linkFramework("CoreFoundation", .{});
        lib_mod.linkFramework("CoreServices", .{});
        lib_mod.linkFramework("SystemConfiguration", .{});
    }

    const with_multi_ssl = false;

    if (target.result.os.tag == .windows) {
        lib_mod.linkSystemLibrary("ws2_32", .{});
        lib_mod.linkSystemLibrary("iphlpapi", .{});
        lib_mod.linkSystemLibrary("bcrypt", .{});
    }

    const mbedtls_dep = mbedtls.build(b, .{
        .target = target,
        .optimize = config.optimize,
    });
    lib_mod.linkLibrary(mbedtls_dep.artifact);
    lib_mod.addCMacro("MBEDTLS_VERSION", "3.6.4");

    const zlib_dep = zlib.build(b, .{
        .target = target,
        .optimize = config.optimize,
    });
    lib_mod.linkLibrary(zlib_dep.artifact);

    const zstd_dep = zstd.build(b, .{
        .target = target,
        .optimize = config.optimize,
    });
    lib_mod.linkLibrary(zstd_dep.artifact);

    const have_lber_h = false;
    const have_ldap_ssl = false;

    // CA handling
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
            var ca_dir = try std.fs.openDirAbsolute(search_ca_path, .{ .iterate = true });
            defer ca_dir.close();

            var ca_dir_it = ca_dir.iterate();
            while (try ca_dir_it.next()) |item| {
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

    const curl_config = b.addConfigHeader(.{
        .style = .{ .cmake = upstream.path("lib/curl_config-cmake.h.in") },
        .include_path = "curl_config.h",
    }, .{
        .CURL_CA_BUNDLE = if (std.mem.eql(u8, ca_bundle, "auto")) null else ca_bundle,
        .CURL_CA_FALLBACK = ca_fallback,
        .CURL_CA_PATH = if (std.mem.eql(u8, ca_path, "auto")) null else ca_path,
        .CURL_DEFAULT_SSL_BACKEND = if (default_ssl_backend) |backend| @tagName(backend) else null,
        .CURL_DISABLE_ALTSVC = disable_altsvc,
        .CURL_DISABLE_COOKIES = disable_cookies,
        .CURL_DISABLE_BASIC_AUTH = disable_basic_auth,
        .CURL_DISABLE_BEARER_AUTH = disable_bearer_auth,
        .CURL_DISABLE_DIGEST_AUTH = disable_digest_auth,
        .CURL_DISABLE_KERBEROS_AUTH = disable_kerberos_auth,
        .CURL_DISABLE_NEGOTIATE_AUTH = disable_negotiate_auth,
        .CURL_DISABLE_AWS = disable_aws,
        .CURL_DISABLE_DICT = disable_dict,
        .CURL_DISABLE_DOH = disable_doh,
        .CURL_DISABLE_FILE = disable_file,
        .CURL_DISABLE_FORM_API = disable_form_api,
        .CURL_DISABLE_FTP = disable_ftp,
        .CURL_DISABLE_GETOPTIONS = disable_getoptions,
        .CURL_DISABLE_GOPHER = disable_gopher,
        .CURL_DISABLE_HEADERS_API = disable_headers_api,
        .CURL_DISABLE_HSTS = disable_hsts,
        .CURL_DISABLE_HTTP = disable_http,
        .CURL_DISABLE_HTTP_AUTH = disable_http_auth,
        .CURL_DISABLE_IMAP = disable_imap,
        .CURL_DISABLE_LDAP = disable_ldap,
        .CURL_DISABLE_LDAPS = disable_ldaps,
        .CURL_DISABLE_LIBCURL_OPTION = disable_libcurl_option,
        .CURL_DISABLE_MIME = disable_mime,
        .CURL_DISABLE_BINDLOCAL = disable_bindlocal,
        .CURL_DISABLE_MQTT = disable_mqtt,
        .CURL_DISABLE_NETRC = disable_netrc,
        .CURL_DISABLE_NTLM = disable_ntlm,
        .CURL_DISABLE_PARSEDATE = disable_parsedate,
        .CURL_DISABLE_POP3 = disable_pop3,
        .CURL_DISABLE_PROGRESS_METER = disable_progress_meter,
        .CURL_DISABLE_PROXY = disable_proxy,
        .CURL_DISABLE_IPFS = disable_ipfs,
        .CURL_DISABLE_RTSP = disable_rtsp,
        .CURL_DISABLE_SHA512_256 = disable_sha512_256,
        .CURL_DISABLE_SHUFFLE_DNS = disable_shuffle_dns,
        .CURL_DISABLE_SMB = disable_smb,
        .CURL_DISABLE_SMTP = disable_smtp,
        .CURL_DISABLE_WEBSOCKETS = disable_websockets,
        .CURL_DISABLE_SOCKETPAIR = disable_socketpair,
        .CURL_DISABLE_TELNET = disable_telnet,
        .CURL_DISABLE_TFTP = disable_tftp,
        .CURL_DISABLE_VERBOSE_STRINGS = disable_verbose_strings,
        .CURL_DISABLE_CA_SEARCH = disable_ca_search,
        .CURL_CA_SEARCH_SAFE = ca_search_safe,
        .CURL_EXTERN_SYMBOL = "__attribute__((__visibility__(\"default\")))",
        .USE_WIN32_CRYPTO = target.result.os.tag == .windows, // Assumes 'NOT WINDOWS_STORE'
        .USE_WIN32_LDAP = target.result.os.tag == .windows and use_win32_ldap and !disable_ldap, // Assumes 'NOT WINDOWS_STORE'
        .USE_IPV6 = enable_ipv6,
        .HAVE_ALARM = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_ARC4RANDOM = switch (target.result.os.tag) {
            .dragonfly,
            .netbsd,
            .freebsd,
            .openbsd,
            .macos,
            .ios,
            .tvos,
            .watchos,
            .visionos,
            .wasi,
            => true,
            else => false,
        },
        .HAVE_ARPA_INET_H = target.result.os.tag != .windows,
        .HAVE_ATOMIC = true,
        .HAVE_ACCEPT4 = switch (target.result.os.tag) {
            .linux => true,
            .freebsd => target.result.os.isAtLeast(.freebsd, .{ .major = 10, .minor = 0, .patch = 0 }) orelse false,
            .netbsd => target.result.os.isAtLeast(.netbsd, .{ .major = 8, .minor = 0, .patch = 0 }) orelse false,
            else => false,
        },
        .HAVE_FNMATCH = target.result.os.tag != .windows,
        .HAVE_BASENAME = true,
        .HAVE_BOOL_T = true,
        .HAVE_BUILTIN_AVAILABLE = target.result.os.tag.isDarwin(),
        .HAVE_CLOCK_GETTIME_MONOTONIC = target.result.os.tag != .windows,
        .HAVE_CLOCK_GETTIME_MONOTONIC_RAW = target.result.os.tag == .linux or target.result.os.tag.isDarwin(),
        .HAVE_CLOSESOCKET = target.result.os.tag == .windows,
        .HAVE_CLOSESOCKET_CAMEL = null,
        .HAVE_DIRENT_H = true,
        .HAVE_OPENDIR = true,
        .HAVE_FCNTL = target.result.os.tag != .windows,
        .HAVE_FCNTL_H = true,
        .HAVE_FCNTL_O_NONBLOCK = target.result.os.tag != .windows,
        .HAVE_FREEADDRINFO = target.result.os.tag != .wasi,
        .HAVE_FSEEKO = target.result.os.tag != .windows,
        .HAVE_DECL_FSEEKO = target.result.os.tag != .windows,
        .HAVE_FTRUNCATE = true,
        .HAVE_GETADDRINFO = target.result.os.tag != .wasi,
        .HAVE_GETADDRINFO_THREADSAFE = target.result.os.tag != .wasi,
        .HAVE_GETEUID = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETPPID = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETHOSTBYNAME_R = switch (target.result.os.tag) {
            .linux => true,
            .windows, .wasi => false,
            .freebsd => target.result.os.isAtLeast(.freebsd, .{ .major = 6, .minor = 2, .patch = 0 }) orelse false,
            .dragonfly => target.result.os.isAtLeast(.dragonfly, .{ .major = 2, .minor = 1, .patch = 0 }) orelse false,
            else => false,
        },
        .HAVE_GETHOSTBYNAME_R_3 = null,
        .HAVE_GETHOSTBYNAME_R_5 = null,
        .HAVE_GETHOSTBYNAME_R_6 = switch (target.result.os.tag) {
            .linux => true,
            .windows, .wasi => false,
            .freebsd => target.result.os.isAtLeast(.freebsd, .{ .major = 6, .minor = 2, .patch = 0 }) orelse false,
            .dragonfly => target.result.os.isAtLeast(.dragonfly, .{ .major = 2, .minor = 1, .patch = 0 }) orelse false,
            else => false,
        },
        .HAVE_GETHOSTNAME = target.result.os.tag != .wasi,
        .HAVE_GETIFADDRS = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETPASS_R = target.result.os.isAtLeast(.netbsd, .{ .major = 7, .minor = 0, .patch = 0 }) orelse false,
        .HAVE_GETPEERNAME = target.result.os.tag != .wasi,
        .HAVE_GETSOCKNAME = target.result.os.tag != .wasi,
        .HAVE_IF_NAMETOINDEX = target.result.os.tag != .wasi,
        .HAVE_GETPWUID = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETPWUID_R = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETRLIMIT = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_GETTIMEOFDAY = true,
        .HAVE_GLIBC_STRERROR_R = false, // TODO why not target.result.isGnuLibC()?
        .HAVE_GMTIME_R = target.result.os.tag != .windows,
        .HAVE_GSSAPI = null,
        .HAVE_GSSGNU = null,
        .CURL_KRB5_VERSION = null,
        .HAVE_IFADDRS_H = target.result.os.tag != .windows,
        .HAVE_INET_NTOP = target.result.os.tag != .windows,
        .HAVE_INET_PTON = target.result.os.tag != .windows,
        .HAVE_SA_FAMILY_T = target.result.os.tag != .windows,
        .HAVE_ADDRESS_FAMILY = target.result.os.tag == .windows,
        .HAVE_IOCTLSOCKET = target.result.os.tag == .windows,
        .HAVE_IOCTLSOCKET_CAMEL = null,
        .HAVE_IOCTLSOCKET_CAMEL_FIONBIO = null,
        .HAVE_IOCTLSOCKET_FIONBIO = target.result.os.tag == .windows,
        .HAVE_IOCTL_FIONBIO = target.result.os.tag != .windows,
        .HAVE_IOCTL_SIOCGIFADDR = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_IO_H = target.result.os.tag == .windows,
        .HAVE_LBER_H = have_lber_h,
        .HAVE_LDAP_SSL = have_ldap_ssl,
        .HAVE_LDAP_SSL_H = null,
        .HAVE_LDAP_URL_PARSE = null,
        .HAVE_LIBGEN_H = true,
        .HAVE_LIBIDN2 = use_libidn2 and !use_apple_idn and !use_win32_idn,
        .HAVE_IDN2_H = use_libidn2 and !use_apple_idn and !use_win32_idn,
        .HAVE_LIBZ = use_zlib,
        .HAVE_BROTLI = use_brotli,
        .HAVE_ZSTD = use_zstd,
        .HAVE_LOCALE_H = true,
        .HAVE_LOCALTIME_R = target.result.os.tag != .windows,
        .HAVE_LONGLONG = true,
        .HAVE_SUSECONDS_T = target.result.os.tag != .windows,
        .HAVE_MSG_NOSIGNAL = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_NETDB_H = target.result.os.tag != .windows,
        .HAVE_NETINET_IN_H = target.result.os.tag != .windows,
        .HAVE_NETINET_IN6_H = null,
        .HAVE_NETINET_TCP_H = target.result.os.tag != .windows,
        .HAVE_NETINET_UDP_H = target.result.os.tag != .windows,
        .HAVE_LINUX_TCP_H = target.result.os.tag == .linux,
        .HAVE_NET_IF_H = target.result.os.tag != .windows,
        .HAVE_OLD_GSSMIT = null,
        .HAVE_PIPE = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_PIPE2 = switch (target.result.os.tag) {
            .linux => true,
            .dragonfly, .freebsd, .netbsd, .openbsd => true,
            else => false,
        },
        .HAVE_EVENTFD = switch (target.result.os.tag) {
            .windows, .wasi => false,
            .linux => if (target.result.isMuslLibC())
                true
            else
                target.result.os.isAtLeast(.linux, .{ .major = 2, .minor = 8, .patch = 0 }),
            else => !target.result.os.tag.isDarwin(),
        },
        .HAVE_POLL = target.result.os.tag != .windows,
        .HAVE_POLL_H = target.result.os.tag != .windows,
        .HAVE_POSIX_STRERROR_R = switch (target.result.os.tag) {
            .windows => false,
            .linux => true, // TODO why not target.result.isMuslLibC()?
            else => true,
        },
        .HAVE_PWD_H = target.result.os.tag != .windows,
        .HAVE_SSL_SET0_WBIO = null, // TODO
        .HAVE_RECV = true,
        .HAVE_SELECT = true,
        .HAVE_SCHED_YIELD = target.result.os.tag != .windows,
        .HAVE_SEND = true,
        .HAVE_SENDMSG = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_SENDMMSG = switch (target.result.os.tag) {
            .windows, .wasi => false,
            .linux => if (target.result.isMuslLibC())
                true
            else
                target.result.os.isAtLeast(.linux, .{ .major = 2, .minor = 14, .patch = 0 }),
            else => !target.result.os.tag.isDarwin(),
        },
        .HAVE_FSETXATTR = target.result.os.tag == .linux or target.result.os.tag == .netbsd,
        .HAVE_FSETXATTR_5 = target.result.os.tag == .linux or target.result.os.tag == .netbsd,
        .HAVE_FSETXATTR_6 = null,
        .HAVE_SETLOCALE = true,
        .HAVE_SETMODE = target.result.os.tag == .windows or target.result.os.tag.isBSD(),
        .HAVE__SETMODE = target.result.os.tag == .windows,
        .HAVE_SETRLIMIT = target.result.os.tag != .wasi,
        .HAVE_SETSOCKOPT_SO_NONBLOCK = null,
        .HAVE_SIGACTION = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_SIGINTERRUPT = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_SIGNAL = target.result.os.tag != .wasi,
        .HAVE_SIGSETJMP = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_SNPRINTF = true,
        .HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID = target.result.os.tag == .windows, // TODO
        .HAVE_SOCKET = target.result.os.tag != .wasi,
        .HAVE_PROTO_BSDSOCKET_H = null,
        .HAVE_SOCKETPAIR = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_STDATOMIC_H = true,
        .HAVE_STDBOOL_H = true,
        .HAVE_STDINT_H = true,
        .HAVE_STRCASECMP = target.result.os.tag != .windows,
        .HAVE_STRCMPI = null,
        .HAVE_STRDUP = true,
        .HAVE_STRERROR_R = target.result.os.tag != .windows,
        .HAVE_STRICMP = null,
        .HAVE_STRINGS_H = true,
        .HAVE_STROPTS_H = target.result.isMuslLibC(),
        .HAVE_MEMRCHR = target.result.os.tag != .windows and !target.result.os.tag.isDarwin() and target.result.os.tag != .wasi,
        .HAVE_STRUCT_SOCKADDR_STORAGE = true,
        .HAVE_STRUCT_TIMEVAL = true,
        .HAVE_SYS_EVENTFD_H = target.result.os.tag != .windows and !target.result.os.tag.isDarwin(),
        .HAVE_SYS_FILIO_H = target.result.os.tag.isBSD(),
        .HAVE_SYS_IOCTL_H = target.result.os.tag != .windows,
        .HAVE_SYS_PARAM_H = true,
        .HAVE_SYS_POLL_H = target.result.os.tag != .windows,
        .HAVE_SYS_RESOURCE_H = target.result.os.tag != .windows and target.result.os.tag != .wasi,
        .HAVE_SYS_SELECT_H = target.result.os.tag != .windows,
        .HAVE_SYS_SOCKIO_H = target.result.os.tag.isBSD(),
        .HAVE_SYS_STAT_H = true,
        .HAVE_SYS_TYPES_H = true,
        .HAVE_SYS_UN_H = target.result.os.tag != .windows,
        .HAVE_SYS_UTIME_H = target.result.os.tag == .windows,
        .HAVE_TERMIOS_H = target.result.os.tag != .windows,
        .HAVE_TERMIO_H = null,
        .HAVE_UNISTD_H = true,
        .HAVE_UTIME = true,
        .HAVE_UTIMES = target.result.os.tag != .windows,
        .HAVE_UTIME_H = true,
        .HAVE_WRITABLE_ARGV = target.result.os.tag.isDarwin(),
        .HAVE_TIME_T_UNSIGNED = null,
        .NEED_REENTRANT = null,
        .CURL_OS = b.fmt("\"{s}\"", .{target.result.zigTriple(b.allocator) catch @panic("OOM")}),
        .SIZEOF_INT_CODE = b.fmt("#define SIZEOF_INT {d}", .{target.result.cTypeByteSize(.int)}),
        .SIZEOF_LONG_CODE = b.fmt("#define SIZEOF_LONG {d}", .{target.result.cTypeByteSize(.long)}),
        .SIZEOF_LONG_LONG_CODE = b.fmt("#define SIZEOF_LONG_LONG {d}", .{target.result.cTypeByteSize(.longlong)}),
        .SIZEOF_OFF_T_CODE = b.fmt("#define SIZEOF_OFF_T {d}", .{8}),
        .SIZEOF_CURL_OFF_T_CODE = b.fmt("#define SIZEOF_CURL_OFF_T {d}", .{8}),
        .SIZEOF_CURL_SOCKET_T_CODE = b.fmt("#define SIZEOF_CURL_SOCKET_T {d}", .{@as(i64, if (target.result.os.tag == .windows) 8 else 4)}),
        .SIZEOF_SIZE_T_CODE = b.fmt("#define SIZEOF_SIZE_T {d}", .{target.result.ptrBitWidth() / 8}),
        .SIZEOF_TIME_T_CODE = b.fmt("#define SIZEOF_TIME_T {d}", .{8}),
        .PACKAGE = "",
        .PACKAGE_BUGREPORT = "curl",
        .PACKAGE_NAME = "a suitable curl mailing list: https://curl.se/mail/",
        .PACKAGE_STRING = "curl",
        .PACKAGE_TARNAME = "curl",
        .PACKAGE_VERSION = b.fmt("{f}", .{version}),
        .STDC_HEADERS = true,
        .USE_ARES = enable_ares,
        .USE_THREADS_POSIX = enable_threaded_resolver and target.result.os.tag != .windows and !target.result.os.tag.isBSD(),
        .USE_THREADS_WIN32 = enable_threaded_resolver and target.result.os.tag == .windows,
        .USE_GNUTLS = use_gnutls,
        .USE_SSLS_EXPORT = use_ssls_export,
        .USE_MBEDTLS = use_mbedtls,
        .USE_RUSTLS = use_rustls,
        .USE_WOLFSSL = use_wolfssl,
        .HAVE_WOLFSSL_DES_ECB_ENCRYPT = false, // TODO
        .HAVE_WOLFSSL_BIO = false, // TODO
        .HAVE_WOLFSSL_FULL_BIO = false, // TODO
        .USE_LIBSSH = use_libssh and !use_libssh2,
        .USE_LIBSSH2 = use_libssh2,
        .USE_LIBPSL = use_libpsl,
        .USE_OPENLDAP = !disable_ldap and !use_win32_ldap, // TODO
        .USE_OPENSSL = use_openssl,
        .USE_AMISSL = null, // AMIGA
        .USE_LIBRTMP = use_librtmp,
        .USE_GSASL = use_gsasl,
        .USE_LIBUV = use_libuv,
        .HAVE_UV_H = use_libuv,
        .CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG = disable_openssl_auto_load_config,
        .USE_NGHTTP2 = use_nghttp2,
        .USE_NGTCP2 = use_ngtcp2,
        .USE_NGHTTP3 = use_ngtcp2, // same condition
        .USE_QUICHE = use_quiche,
        .USE_OPENSSL_QUIC = use_openssl_quic,
        .HAVE_QUICHE_CONN_SET_QLOG_FD = null, // TODO
        .USE_UNIX_SOCKETS = target.result.os.tag == .windows or enable_unix_sockets,
        .USE_WIN32_LARGE_FILES = target.result.os.tag == .windows,
        .USE_WINDOWS_SSPI = enable_windows_sspi,
        .USE_SCHANNEL = use_schannel,
        .USE_WATT32 = null, // DOS
        .CURL_WITH_MULTI_SSL = with_multi_ssl,
        .VERSION = b.fmt("{f}", .{version}),
        ._FILE_OFFSET_BITS = 64,
        ._LARGE_FILES = null, // OS/400
        ._THREAD_SAFE = null, // AIX 4.3
        .@"const" = null,
        .size_t = null,
        .ssize_t = null,
        .HAVE_MACH_ABSOLUTE_TIME = target.result.os.tag.isDarwin(),
        .USE_WIN32_IDN = target.result.os.tag == .windows and use_win32_idn,
        .USE_APPLE_IDN = target.result.os.tag.isDarwin() and use_apple_idn,
        .HAVE_OPENSSL_SRP = null, // TODO
        .HAVE_GNUTLS_SRP = null, // TODO
        .USE_TLS_SRP = null, // TODO
        .USE_HTTPSRR = httpsrr,
        .USE_ECH = ech,
        .HAVE_WOLFSSL_CTX_GENERATEECHCONFIG = null, // TODO
        .HAVE_SSL_SET1_ECH_CONFIG_LIST = null, // TODO
        .HAVE_DES_ECB_ENCRYPT = false, // TODO
    });
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

/// `LIB_CURLX_CFILES` in `lib/Makefile.inc`.
const lib_curlx_sources: []const []const u8 = &.{
    "curlx/base64.c",
    "curlx/dynbuf.c",
    "curlx/fopen.c",
    "curlx/inet_ntop.c",
    "curlx/inet_pton.c",
    "curlx/multibyte.c",
    "curlx/nonblock.c",
    "curlx/strcopy.c",
    "curlx/strerr.c",
    "curlx/strparse.c",
    "curlx/timediff.c",
    "curlx/timeval.c",
    "curlx/version_win32.c",
    "curlx/wait.c",
    "curlx/warnless.c",
    "curlx/winapi.c",
};

/// `LIB_VAUTH_CFILES` in `lib/Makefile.inc`.
const lib_vauth_sources: []const []const u8 = &.{
    "vauth/cleartext.c",
    "vauth/cram.c",
    "vauth/digest.c",
    "vauth/digest_sspi.c",
    "vauth/gsasl.c",
    "vauth/krb5_gssapi.c",
    "vauth/krb5_sspi.c",
    "vauth/ntlm.c",
    "vauth/ntlm_sspi.c",
    "vauth/oauth2.c",
    "vauth/spnego_gssapi.c",
    "vauth/spnego_sspi.c",
    "vauth/vauth.c",
};

const lib_vtls_sources: []const []const u8 = &.{
    "vtls/apple.c",
    "vtls/cipher_suite.c",
    "vtls/gtls.c",
    "vtls/hostcheck.c",
    "vtls/keylog.c",
    "vtls/mbedtls.c",
    "vtls/openssl.c",
    "vtls/rustls.c",
    "vtls/schannel.c",
    "vtls/schannel_verify.c",
    "vtls/vtls.c",
    "vtls/vtls_scache.c",
    "vtls/vtls_spack.c",
    "vtls/wolfssl.c",
    "vtls/x509asn1.c",
};

/// `LIB_VQUIC_CFILES` in `lib/Makefile.inc`.
const lib_vquic_sources: []const []const u8 = &.{
    "vquic/curl_ngtcp2.c",
    "vquic/curl_osslq.c",
    "vquic/curl_quiche.c",
    "vquic/vquic.c",
    "vquic/vquic-tls.c",
};

/// `LIB_VSSH_CFILES` in `lib/Makefile.inc`.
const lib_vssh_sources: []const []const u8 = &.{
    "vssh/libssh.c",
    "vssh/libssh2.c",
    "vssh/vssh.c",
};

/// `LIB_CFILES` in `lib/Makefile.inc`.
const lib_sources: []const []const u8 = &.{
    "altsvc.c",
    "amigaos.c",
    "asyn-ares.c",
    "asyn-base.c",
    "asyn-thrdd.c",
    "bufq.c",
    "bufref.c",
    "cf-h1-proxy.c",
    "cf-h2-proxy.c",
    "cf-haproxy.c",
    "cf-https-connect.c",
    "cf-ip-happy.c",
    "cf-socket.c",
    "cfilters.c",
    "conncache.c",
    "connect.c",
    "content_encoding.c",
    "cookie.c",
    "cshutdn.c",
    "curl_addrinfo.c",
    "curl_endian.c",
    "curl_fnmatch.c",
    "curl_fopen.c",
    "curl_get_line.c",
    "curl_gethostname.c",
    "curl_gssapi.c",
    "curl_memrchr.c",
    "curl_ntlm_core.c",
    "curl_range.c",
    "curl_rtmp.c",
    "curl_sasl.c",
    "curl_sha512_256.c",
    "curl_share.c",
    "curl_sspi.c",
    "curl_threads.c",
    "curl_trc.c",
    "cw-out.c",
    "cw-pause.c",
    "dict.c",
    "doh.c",
    "dynhds.c",
    "easy.c",
    "easygetopt.c",
    "easyoptions.c",
    "escape.c",
    "fake_addrinfo.c",
    "file.c",
    "fileinfo.c",
    "formdata.c",
    "ftp.c",
    "ftplistparser.c",
    "getenv.c",
    "getinfo.c",
    "gopher.c",
    "hash.c",
    "headers.c",
    "hmac.c",
    "hostip.c",
    "hostip4.c",
    "hostip6.c",
    "hsts.c",
    "http.c",
    "http1.c",
    "http2.c",
    "http_aws_sigv4.c",
    "http_chunks.c",
    "http_digest.c",
    "http_negotiate.c",
    "http_ntlm.c",
    "http_proxy.c",
    "httpsrr.c",
    "idn.c",
    "if2ip.c",
    "imap.c",
    "ldap.c",
    "llist.c",
    "macos.c",
    "md4.c",
    "md5.c",
    "memdebug.c",
    "mime.c",
    "mprintf.c",
    "mqtt.c",
    "multi.c",
    "multi_ev.c",
    "multi_ntfy.c",
    "netrc.c",
    "noproxy.c",
    "openldap.c",
    "parsedate.c",
    "pingpong.c",
    "pop3.c",
    "progress.c",
    "psl.c",
    "rand.c",
    "ratelimit.c",
    "request.c",
    "rtsp.c",
    "select.c",
    "sendf.c",
    "setopt.c",
    "sha256.c",
    "slist.c",
    "smb.c",
    "smtp.c",
    "socketpair.c",
    "socks.c",
    "socks_gssapi.c",
    "socks_sspi.c",
    "splay.c",
    "strcase.c",
    "strdup.c",
    "strequal.c",
    "strerror.c",
    "system_win32.c",
    "telnet.c",
    "tftp.c",
    "transfer.c",
    "uint-bset.c",
    "uint-hash.c",
    "uint-spbset.c",
    "uint-table.c",
    "url.c",
    "urlapi.c",
    "version.c",
    "ws.c",
};

/// `CSOURCES` in `lib/Makefile.inc`.
const sources = lib_sources ++ lib_vauth_sources ++ lib_vtls_sources ++ lib_vquic_sources ++ lib_vssh_sources ++ lib_curlx_sources;

/// `CURLX_CFILES` in `src/Makefile.inc`.
const curlx_sources: []const []const u8 = &.{
    "curlx/base64.c",
    "curlx/dynbuf.c",
    "curlx/fopen.c",
    "curlx/multibyte.c",
    "curlx/nonblock.c",
    "curlx/strcopy.c",
    "curlx/strerr.c",
    "curlx/strparse.c",
    "curlx/timediff.c",
    "curlx/timeval.c",
    "curlx/version_win32.c",
    "curlx/wait.c",
    "curlx/warnless.c",
    "curlx/winapi.c",
};

/// `CURLX_HFILES` in `src/Makefile.inc`.
const curlx_headers: []const []const u8 = &.{
    "curl_setup.h",
    "curlx/binmode.h",
    "curlx/dynbuf.h",
    "curlx/fopen.h",
    "curlx/multibyte.h",
    "curlx/nonblock.h",
    "curlx/snprintf.h",
    "curlx/strcopy.h",
    "curlx/strerr.h",
    "curlx/strparse.h",
    "curlx/timediff.h",
    "curlx/timeval.h",
    "curlx/version_win32.h",
    "curlx/wait.h",
    "curlx/warnless.h",
    "curlx/winapi.h",
};

/// `CURL_CFILES` in `src/Makefile.inc`.
const exe_sources: []const []const u8 = &.{
    "config2setopts.c",
    "slist_wc.c",
    "terminal.c",
    "tool_bname.c",
    "tool_cb_dbg.c",
    "tool_cb_hdr.c",
    "tool_cb_prg.c",
    "tool_cb_rea.c",
    "tool_cb_see.c",
    "tool_cb_soc.c",
    "tool_cb_wrt.c",
    "tool_cfgable.c",
    "tool_dirhie.c",
    "tool_doswin.c",
    "tool_easysrc.c",
    "tool_filetime.c",
    "tool_findfile.c",
    "tool_formparse.c",
    "tool_getparam.c",
    "tool_getpass.c",
    "tool_help.c",
    "tool_helpers.c",
    "tool_ipfs.c",
    "tool_libinfo.c",
    "tool_listhelp.c",
    "tool_main.c",
    "tool_msgs.c",
    "tool_operate.c",
    "tool_operhlp.c",
    "tool_paramhlp.c",
    "tool_parsecfg.c",
    "tool_progress.c",
    "tool_setopt.c",
    "tool_ssls.c",
    "tool_stderr.c",
    "tool_strdup.c",
    "tool_urlglob.c",
    "tool_util.c",
    "tool_vms.c",
    "tool_writeout.c",
    "tool_writeout_json.c",
    "tool_xattr.c",
    "var.c",
    "toolx/tool_time.c",
};
