const std = @import("std");

extern fn launch(argc: i32, argv: [*]const [:0]const u8) i32;

var gpa: std.heap.GeneralPurposeAllocator(.{
    .thread_safe = true,
}) = .init;
const allocator = gpa.allocator();

pub fn main() !void {
    const args = try std.process.argsAlloc(std.heap.page_allocator);
    defer std.process.argsFree(std.heap.page_allocator, args);
    const result = launch(@intCast(args.len), args.ptr);

    const leaked = if (result == 0) gpa.detectLeaks() else false;
    std.process.exit(@intFromBool(leaked));
}

const alignment = 16;
const Header = struct {
    size: usize,
    offset: usize,
};

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    const total = size + alignment + @sizeOf(Header);

    const mem = allocator.alloc(u8, total) catch return null;
    const base_ptr = mem.ptr;

    const aligned_ptr = std.mem.alignForward(
        usize,
        @intFromPtr(base_ptr) + @sizeOf(Header),
        alignment,
    );

    const header = @as(
        *Header,
        @ptrFromInt(aligned_ptr - @sizeOf(Header)),
    );
    header.size = total;
    header.offset = aligned_ptr - @intFromPtr(base_ptr);
    return @ptrFromInt(aligned_ptr);
}

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    const p = ptr orelse return;
    const header = @as(
        *Header,
        @ptrFromInt(@intFromPtr(p) - @sizeOf(Header)),
    );

    const base_ptr = @intFromPtr(p) - header.offset;
    const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
    allocator.free(slice);
}
