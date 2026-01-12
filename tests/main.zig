const std = @import("std");

extern fn launch(argc: i32, argv: [*]const [:0]const u8) i32;

var gpa: std.heap.GeneralPurposeAllocator(.{
    .thread_safe = true,
}) = .init;

const proc_allocator = gpa.allocator();

const Instrumentor = struct {
    const internal_allocator = std.heap.c_allocator;

    total_nodes: std.atomic.Value(u64),
    total_alloc: std.atomic.Value(u64),
    node_counter: std.atomic.Value(u64),
    byte_counter: std.atomic.Value(u64),

    live_lock: std.Thread.Mutex,
    live_allocations: std.AutoHashMap(usize, void),

    pub fn initOnce() void {
        instrumentor = .init();
    }

    pub fn init() Instrumentor {
        return .{
            .total_nodes = .init(0),
            .total_alloc = .init(0),
            .node_counter = .init(0),
            .byte_counter = .init(0),
            .live_lock = .{},
            .live_allocations = .init(internal_allocator),
        };
    }

    pub fn deinit(self: *Instrumentor) void {
        self.live_allocations.deinit();
    }

    pub fn putKey(self: *Instrumentor, key: usize) ?usize {
        self.live_lock.lock();
        defer self.live_lock.unlock();

        if (self.live_allocations.contains(key)) return null;
        self.live_allocations.put(key, {}) catch return null;
        return key;
    }

    pub fn removeKey(self: *Instrumentor, key: usize) bool {
        self.live_lock.lock();
        defer self.live_lock.unlock();
        return self.live_allocations.remove(key);
    }

    pub fn report(self: *Instrumentor) void {
        const allocated, const alloc_unit = formatBytes(self.total_alloc.load(.acquire));
        const remaining, const rem_unit = formatBytes(self.byte_counter.load(.acquire));

        std.log.info("{d} nodes malloced for {d} {s}", .{
            self.total_nodes.load(.acquire),
            allocated,
            alloc_unit,
        });
        std.log.info("{d} leak(s) for {d} total leaked {s}", .{
            self.node_counter.load(.acquire),
            remaining,
            rem_unit,
        });
    }

    fn formatBytes(bytes: u64) struct { u64, []const u8 } {
        if (bytes > 1_000_000_000) return .{ bytes / 1_000_000_000, "GB" };
        if (bytes > 1_000_000) return .{ bytes / 1_000_000, "MB" };
        if (bytes > 1_000) return .{ bytes / 1_000, "KB" };
        return .{ bytes, "bytes" };
    }
};

var instrumentor: Instrumentor = undefined;
var instrumentor_once = std.once(Instrumentor.initOnce);

pub fn main() !void {
    instrumentor_once.call();
    defer instrumentor.deinit();

    const proc = blk: {
        const args = try std.process.argsAlloc(Instrumentor.internal_allocator);
        defer std.process.argsFree(Instrumentor.internal_allocator, args);
        break :blk launch(@intCast(args.len), args.ptr);
    };

    const result: u8 = @intCast(@intFromBool(gpa.detectLeaks()) | proc);
    instrumentor.report();
    std.process.exit(result);
}

const Header = extern struct {
    size: usize,
    offset: usize,
    requested: usize,
};

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    instrumentor_once.call();
    const alignment = comptime @max(16, @alignOf(std.c.max_align_t));
    const total = size + alignment + @sizeOf(Header);

    _ = instrumentor.total_nodes.fetchAdd(1, .acq_rel);
    _ = instrumentor.total_alloc.fetchAdd(@intCast(total), .acq_rel);
    _ = instrumentor.node_counter.fetchAdd(1, .acq_rel);

    const mem = proc_allocator.alloc(u8, total) catch return null;
    const base_ptr = mem.ptr;

    const aligned_ptr = blk: {
        const ptr = std.mem.alignForward(
            usize,
            @intFromPtr(base_ptr) + @sizeOf(Header),
            alignment,
        );

        break :blk instrumentor.putKey(ptr) orelse return null;
    };

    const header = @as(
        *Header,
        @ptrFromInt(aligned_ptr - @sizeOf(Header)),
    );
    header.* = .{
        .size = total,
        .offset = aligned_ptr - @intFromPtr(base_ptr),
        .requested = size,
    };
    _ = instrumentor.byte_counter.fetchAdd(header.requested, .acq_rel);
    return @ptrFromInt(aligned_ptr);
}

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    instrumentor_once.call();
    const p = ptr orelse return;
    if (!instrumentor.removeKey(@intFromPtr(p))) {
        @panic("Double or invalid free detected");
    }

    const header = blk: {
        const header = @as(
            *const Header,
            @ptrFromInt(@intFromPtr(p) - @sizeOf(Header)),
        );

        if (header.offset > header.size) {
            @panic("Heap corruption detected: allocated block has malformed header");
        }
        break :blk header;
    };

    _ = instrumentor.byte_counter.fetchSub(header.requested, .acq_rel);
    _ = instrumentor.node_counter.fetchSub(1, .acq_rel);

    const base_ptr = @intFromPtr(p) - header.offset;
    const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
    proc_allocator.free(slice);
}
