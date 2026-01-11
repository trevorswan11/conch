const std = @import("std");

extern fn launch(argc: i32, argv: [*]const [:0]const u8) i32;

var gpa: std.heap.GeneralPurposeAllocator(.{
    .thread_safe = true,
}) = .init;
const allocator = gpa.allocator();

var total_nodes: std.atomic.Value(u64) = .init(0);
var total_alloc: std.atomic.Value(u64) = .init(0);
var node_counter: std.atomic.Value(u64) = .init(0);
var byte_counter: std.atomic.Value(u64) = .init(0);

pub fn main() !void {
    const internal_allocator = std.heap.c_allocator;
    const args = try std.process.argsAlloc(internal_allocator);
    defer std.process.argsFree(internal_allocator, args);
    const proc = launch(@intCast(args.len), args.ptr);

    const nodes = total_nodes.load(.acquire);
    const nodes_leaked: u64 = node_counter.load(.acquire);

    const allocated_bytes = total_alloc.load(.acquire);
    const remaining_bytes = byte_counter.load(.acquire);
    const allocated, const alloc_unit = formatBytes(allocated_bytes);
    const remaining, const rem_unit = formatBytes(remaining_bytes);

    const has_leaks = if (proc == 0) gpa.detectLeaks() else false;
    const result: u8 = @intCast(@intFromBool(has_leaks) | proc);

    std.debug.print("Process: {d} nodes malloced for {d} {s}\n", .{ nodes, allocated, alloc_unit });
    std.debug.print("Process: {d} leaks for {d} total leaked {s}\n", .{ nodes_leaked, remaining, rem_unit });

    std.process.exit(result);
}

fn formatBytes(bytes: u64) struct { u64, []const u8 } {
    if (bytes > 1_000_000_000) return .{ bytes / 1_000_000_000, "GB" };
    if (bytes > 1_000_000) return .{ bytes / 1_000_000, "MB" };
    if (bytes > 1_000) return .{ bytes / 1_000, "KB" };
    return .{ bytes, "bytes" };
}

const alignment = 16;
const Header = struct {
    size: usize,
    offset: usize,
};

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    const total = size + alignment + @sizeOf(Header);
    _ = total_nodes.fetchAdd(1, .acquire);
    _ = node_counter.fetchAdd(1, .acquire);
    _ = total_alloc.fetchAdd(@intCast(total), .acquire);

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
    _ = byte_counter.fetchAdd(header.size - header.offset, .acquire);
    return @ptrFromInt(aligned_ptr);
}

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    const p = ptr orelse return;
    const header = @as(
        *Header,
        @ptrFromInt(@intFromPtr(p) - @sizeOf(Header)),
    );
    _ = byte_counter.fetchSub(header.size - header.offset, .acquire);
    _ = node_counter.fetchSub(1, .acquire);

    const base_ptr = @intFromPtr(p) - header.offset;
    const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
    allocator.free(slice);
}
