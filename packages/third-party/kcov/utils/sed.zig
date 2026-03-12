//! https://github.com/allyourcodebase/binutils/blob/master/find_replace.zig
const std = @import("std");

pub fn main() !void {
    var debug_allocator: std.heap.DebugAllocator(.{}) = .init;
    defer _ = debug_allocator.deinit();
    const gpa = debug_allocator.allocator();

    const args = try std.process.argsAlloc(gpa);
    defer std.process.argsFree(gpa, args);

    if (args.len != 5) {
        std.debug.print("usage: {s} <input_file> <output_file> <before> <after>\n", .{args[0]});
        return error.UsageError;
    }

    const input_filename = args[1];
    const output_filename = args[2];
    const before = args[3];
    const after = args[4];

    const input = try std.fs.cwd().readFileAlloc(gpa, input_filename, std.math.maxInt(usize));
    defer gpa.free(input);

    const output = try std.mem.replaceOwned(u8, gpa, input, before, after);
    defer gpa.free(output);

    try std.fs.cwd().writeFile(.{
        .sub_path = output_filename,
        .data = output,
    });
}
