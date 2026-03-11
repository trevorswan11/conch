const std = @import("std");

const Dependency = @import("Dependency.zig");
const Config = Dependency.Config;

/// Compiles mbedtls from source as a static library.
/// https://github.com/allyourcodebase/mbedtls
pub fn build(b: *std.Build, config: Config) Dependency {
    const upstream = b.dependency("mbedtls", .{});
    const target = config.target;
    const mod = b.createModule(.{
        .target = target,
        .optimize = config.optimize,
        .link_libc = true,
    });

    mod.addIncludePath(upstream.path("include"));
    mod.addCSourceFiles(.{
        .root = upstream.path("library"),
        .files = sources,
    });

    if (target.result.os.tag == .freebsd) {
        mod.addCMacro("__BSD_VISIBLE", "1");
    }
    mod.addCMacro("MBEDTLS_THREADING_C", "");
    mod.addCMacro("MBEDTLS_THREADING_PTHREAD", "");

    if (target.result.os.tag == .windows) {
        mod.linkSystemLibrary("bcrypt", .{});
    }

    const mbedtls = b.addLibrary(.{
        .name = "mbedtls",
        .root_module = mod,
    });
    mbedtls.installHeadersDirectory(upstream.path("include/mbedtls"), "mbedtls", .{});
    mbedtls.installHeadersDirectory(upstream.path("include/psa"), "psa", .{});

    return .{
        .upstream = upstream,
        .artifact = mbedtls,
    };
}

const sources: []const []const u8 = &.{
    "x509_create.c",
    "x509_crt.c",
    "psa_crypto_client.c",
    "aes.c",
    "psa_crypto_slot_management.c",
    "bignum_mod_raw.c",
    "psa_crypto_driver_wrappers_no_static.c",
    "camellia.c",
    "constant_time.c",
    "pk_wrap.c",
    "pk.c",
    "pkcs7.c",
    "aesce.c",
    "ssl_tls13_client.c",
    "ssl_tls12_client.c",
    "psa_util.c",
    "ecdh.c",
    "ssl_tls.c",
    "x509_crl.c",
    "cipher_wrap.c",
    "chacha20.c",
    "psa_crypto_rsa.c",
    "des.c",
    "ssl_cookie.c",
    "ctr_drbg.c",
    "psa_crypto_mac.c",
    "aesni.c",
    "dhm.c",
    "ssl_cache.c",
    "ssl_ciphersuites.c",
    "ecp_curves_new.c",
    "hmac_drbg.c",
    "rsa.c",
    "ssl_ticket.c",
    "asn1parse.c",
    "mps_trace.c",
    "pkwrite.c",
    "gcm.c",
    "sha1.c",
    "ssl_client.c",
    "asn1write.c",
    "ccm.c",
    "version_features.c",
    "aria.c",
    "lms.c",
    "psa_crypto_cipher.c",
    "entropy_poll.c",
    "x509write_csr.c",
    "platform.c",
    "cmac.c",
    "bignum.c",
    "pkparse.c",
    "psa_crypto_ffdh.c",
    "ssl_msg.c",
    "debug.c",
    "ripemd160.c",
    "pkcs5.c",
    "ssl_tls13_generic.c",
    "x509write.c",
    "bignum_mod.c",
    "pem.c",
    "oid.c",
    "error.c",
    "psa_crypto_pake.c",
    "x509_csr.c",
    "psa_its_file.c",
    "psa_crypto.c",
    "rsa_alt_helpers.c",
    "ssl_debug_helpers_generated.c",
    "platform_util.c",
    "psa_crypto_se.c",
    "base64.c",
    "memory_buffer_alloc.c",
    "mps_reader.c",
    "psa_crypto_aead.c",
    "ecp.c",
    "lmots.c",
    "version.c",
    "x509.c",
    "bignum_core.c",
    "chachapoly.c",
    "ssl_tls13_keys.c",
    "sha256.c",
    "ecp_curves.c",
    "md5.c",
    "timing.c",
    "psa_crypto_ecp.c",
    "psa_crypto_storage.c",
    "poly1305.c",
    "x509write_crt.c",
    "hkdf.c",
    "sha3.c",
    "threading.c",
    "padlock.c",
    "psa_crypto_hash.c",
    "pkcs12.c",
    "entropy.c",
    "ssl_tls13_server.c",
    "ssl_tls12_server.c",
    "net_sockets.c",
    "sha512.c",
    "md.c",
    "ecjpake.c",
    "cipher.c",
    "ecdsa.c",
    "nist_kw.c",
    "pk_ecc.c",
};
