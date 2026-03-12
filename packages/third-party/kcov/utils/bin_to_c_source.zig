//! This is bin-to-c-source.py ported to Zig:
//! https://github.com/SimonKagstrom/kcov/blob/master/src/bin-to-c-source.py
//! https://github.com/allyourcodebase/kcov/blob/master/bin_to_c_source.zig
const std = @import("std");

pub fn main() !void {
    var debug_allocator: std.heap.DebugAllocator(.{}) = .init;
    defer _ = debug_allocator.deinit();
    const gpa = debug_allocator.allocator();

    const args = try std.process.argsAlloc(gpa);
    defer std.process.argsFree(gpa, args);

    var buffer: [4096]u8 = undefined;
    var stdout_writer = std.fs.File.stdout().writer(&buffer);
    const stdout = &stdout_writer.interface;

    var stderr_writer = std.fs.File.stderr().writer(&.{});
    const stderr = &stderr_writer.interface;

    if (args.len < 3 or (args.len - 1) % 2 != 0) {
        try stderr.print("Usage: {s} <file> <base-name> [<file2> <base-name2>]\n", .{args[0]});
        std.process.exit(1);
    }

    try stdout.writeAll(
        \\#include <stdint.h>
        \\#include <stdlib.h>
        \\#include <generated-data-base.hh>
        \\using namespace kcov;
        \\
    );

    var i: usize = 1;
    while (i + 1 < args.len) : (i += 2) {
        const file = args[i];
        const base_name = args[i + 1];

        const data = try std.fs.cwd().readFileAlloc(gpa, file, std.math.maxInt(usize));
        defer gpa.free(data);

        try generate(stdout, data, base_name);
    }

    try stdout.flush();
}

fn generate(writer: *std.Io.Writer, data: []const u8, base_name: []const u8) std.Io.Writer.Error!void {
    try writer.print("const uint8_t {s}_data_raw[] = {{\n", .{base_name});

    for (data, 0..) |c, i| {
        // more optimized version of:
        // try writer.print("0x{x:0>2},", .{c});
        const charset = "0123456789abcdef";
        try writer.writeAll(&.{ '0', 'x', charset[c >> 4], charset[c & 15], ',' });

        if (i % 20 == 19) try writer.writeByte('\n');
    }

    try writer.print(
        \\
        \\}};
        \\GeneratedData {0s}_data({0s}_data_raw, sizeof({0s}_data_raw));
        \\
    , .{base_name});
}
