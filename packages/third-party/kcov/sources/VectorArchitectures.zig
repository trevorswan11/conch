//! https://github.com/allyourcodebase/mbedtls/blob/main/build.zig
const std = @import("std");

const Self = @This();

default_vector: []const u8,
select_vectors: []const []const u8,
select_architectures: []const []const u8,

pub fn init(target: std.Build.ResolvedTarget) Self {
    const default_vector: []const u8, const select_vectors: []const []const u8, const select_architectures: []const []const u8 = switch (target.result.cpu.arch) {
        .x86_64 => switch (target.result.os.tag) {
            .windows => .{
                "x86_64_elf64_vec",
                &.{ "i386_elf32_vec", "iamcu_elf32_vec", "x86_64_elf32_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .macos => .{
                "x86_64_mach_o_vec",
                &.{ "i386_mach_o_vec", "mach_o_le_vec", "mach_o_be_vec", "mach_o_fat_vec", "pef_vec", "pef_xlib_vec", "sym_vec" },
                &.{ "bfd_i386_arch", "bfd_powerpc_arch", "bfd_rs6000_arch" },
            },
            .linux => .{
                "x86_64_elf64_vec",
                &.{ "i386_elf32_vec", "iamcu_elf32_vec", "x86_64_elf32_vec", "i386_pei_vec", "x86_64_pe_vec", "x86_64_pei_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .freebsd => .{
                "x86_64_elf64_fbsd_vec",
                &.{ "i386_elf32_fbsd_vec", "iamcu_elf32_vec", "i386_coff_vec", "i386_pei_vec", "x86_64_pe_vec", "x86_64_pei_vec", "i386_elf32_vec", "x86_64_elf64_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .netbsd => .{
                "x86_64_elf64_vec",
                &.{ "i386_elf32_vec", "iamcu_elf32_vec", "i386_coff_vec", "i386_pei_vec", "x86_64_pe_vec", "x86_64_pei_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
        },
        .x86 => switch (target.result.os.tag) {
            .windows => .{
                "i386_elf32_vec",
                &.{ "iamcu_elf32_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .linux => .{
                "i386_elf32_vec",
                &.{ "iamcu_elf32_vec", "i386_pei_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .freebsd => .{
                "i386_elf32_fbsd_vec",
                &.{ "i386_elf32_vec", "iamcu_elf32_vec", "i386_pei_vec", "i386_coff_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            .netbsd => .{
                "i386_elf32_vec",
                &.{ "iamcu_elf32_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_i386_arch", "bfd_iamcu_arch" },
            },
            else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
        },
        .aarch64 => switch (target.result.os.tag) {
            .windows => .{
                "aarch64_elf64_le_vec",
                &.{ "aarch64_elf64_be_vec", "aarch64_elf32_le_vec", "aarch64_elf32_be_vec", "arm_elf32_le_vec", "arm_elf32_be_vec", "aarch64_pei_le_vec", "aarch64_pe_le_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_aarch64_arch", "bfd_arm_arch" },
            },
            .macos => .{
                "x86_64_mach_o_vec",
                &.{ "i386_mach_o_vec", "mach_o_le_vec", "mach_o_be_vec", "mach_o_fat_vec", "pef_vec", "pef_xlib_vec", "sym_vec" },
                &.{ "bfd_aarch64_arch", "bfd_arm_arch", "bfd_i386_arch", "bfd_powerpc_arch", "bfd_rs6000_arch" },
            },
            .linux => .{
                "x86_64_elf64_vec",
                &.{ "i386_elf32_vec", "iamcu_elf32_vec", "x86_64_elf32_vec", "i386_pei_vec", "x86_64_pe_vec", "x86_64_pei_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_aarch64_arch", "bfd_arm_arch" },
            },
            .freebsd => .{
                "aarch64_elf64_le_vec",
                &.{ "aarch64_elf64_be_vec", "arm_elf32_le_vec", "arm_elf32_be_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_aarch64_arch", "bfd_arm_arch" },
            },
            .netbsd => .{
                "aarch64_elf64_le_vec",
                &.{ "aarch64_elf64_be_vec", "aarch64_elf32_le_vec", "aarch64_elf32_be_vec", "arm_elf32_le_vec", "arm_elf32_be_vec", "aarch64_pei_le_vec", "aarch64_pe_le_vec", "elf64_le_vec", "elf64_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{ "bfd_aarch64_arch", "bfd_arm_arch" },
            },
            else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
        },
        .arm => switch (target.result.os.tag) {
            .linux => .{
                "arm_elf32_le_vec",
                &.{ "arm_elf32_fdpic_le_vec", "arm_elf32_be_vec", "arm_elf32_fdpic_be_vec", "elf32_le_vec", "elf32_be_vec" },
                &.{"bfd_arm_arch"},
            },
            else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
        },
        .wasm32 => switch (target.result.os.tag) {
            .wasi => .{
                "wasm_vec",
                &.{ "elf32_le_vec", "elf32_be_vec" },
                &.{"bfd_wasm32_arch"},
            },
            else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
        },
        else => std.debug.panic("TODO '{s}-{s}'", .{ @tagName(target.result.cpu.arch), @tagName(target.result.os.tag) }),
    };

    return .{
        .default_vector = default_vector,
        .select_architectures = select_architectures,
        .select_vectors = select_vectors,
    };
}
