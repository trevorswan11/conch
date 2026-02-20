const std = @import("std");
const testing = std.testing;

extern fn launch(c_int, [*c][*c]u8) i32;

var instrumentor: Instrumentor = undefined;

pub fn main() !void {
    Instrumentor.once.call();
    defer instrumentor.deinit();

    const args = try instrumentor.getCArgs();
    std.log.info("{s}", .{args.argv[0]});
    const proc = launch(args.argc, args.argv);

    const result: u8 = @intCast(@intFromBool(instrumentor.tryDumpLeaks()) | proc);
    instrumentor.report();
    std.process.exit(result);
}

const Instrumentor = struct {
    const internal_allocator = std.heap.c_allocator;

    const AllocHeader = extern struct {
        size: usize,
        offset: usize,
        requested: usize,
        magic: usize = header_magic,

        pub fn valid(self: *const AllocHeader) bool {
            return self.offset < self.size and self.magic == header_magic;
        }
    };

    const header_magic = 0xDEADBEEF;
    const header_size = @sizeOf(AllocHeader);

    var once = std.once(initOnce);

    gpa: std.heap.DebugAllocator(.{
        .thread_safe = true,
        .stack_trace_frames = if (std.debug.sys_can_stack_trace) 10 else 0,
    }),

    args: ?struct {
        zig_conv: [][:0]u8,
        c_conv: [][*c]u8,
    } = null,

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
            .gpa = .init,

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
        if (self.args) |args| {
            std.process.argsFree(Instrumentor.internal_allocator, args.zig_conv);
            Instrumentor.internal_allocator.free(args.c_conv);
        }
        _ = self.gpa.deinit();
    }

    pub fn getCArgs(self: *Instrumentor) !struct { argc: c_int, argv: [*c][*c]u8 } {
        const process_args = try std.process.argsAlloc(Instrumentor.internal_allocator);
        errdefer std.process.argsFree(Instrumentor.internal_allocator, process_args);
        const c_args = try Instrumentor.internal_allocator.alloc([*c]u8, process_args.len);
        errdefer Instrumentor.internal_allocator.free(c_args);

        for (process_args, 0..) |arg, idx| {
            c_args[idx] = arg.ptr;
        }

        self.args = .{ .zig_conv = process_args, .c_conv = c_args };
        return .{ .argc = @intCast(c_args.len), .argv = c_args.ptr };
    }

    pub fn allocator(self: *Instrumentor) std.mem.Allocator {
        return self.gpa.allocator();
    }

    // Handles the locks itself
    pub fn putKey(self: *Instrumentor, key: usize) ?usize {
        self.live_lock.lock();
        defer self.live_lock.unlock();

        if (self.live_allocations.contains(key)) return null;
        self.live_allocations.put(key, {}) catch return null;
        return key;
    }

    // Does not handle the locks itself!
    pub fn containsKey(self: *Instrumentor, key: usize) bool {
        return self.live_allocations.contains(key);
    }

    // Does not handle the locks itself!
    pub fn removeKey(self: *Instrumentor, key: usize) bool {
        return self.live_allocations.remove(key);
    }

    pub fn tryDumpLeaks(self: *Instrumentor) bool {
        return self.gpa.detectLeaks();
    }

    pub fn report(self: *Instrumentor) void {
        const allocated, const alloc_unit = formatBytes(self.total_alloc.load(.acquire));
        const remaining, const rem_unit = formatBytes(self.byte_counter.load(.acquire));

        std.log.info("{d} nodes malloced for {d:.3} {s}", .{
            self.total_nodes.load(.acquire),
            allocated,
            alloc_unit,
        });
        std.log.info("{d} leak(s) for {d:.3} total leaked {s}\n", .{
            self.node_counter.load(.acquire),
            remaining,
            rem_unit,
        });
    }

    fn formatBytes(bytes: u64) struct { f64, []const u8 } {
        const float_bytes: f64 = @floatFromInt(bytes);
        if (bytes >= 1_000_000_000) return .{ float_bytes / 1_000_000_000.0, "GB" };
        if (bytes >= 1_000_000) return .{ float_bytes / 1_000_000.0, "MB" };
        if (bytes >= 1_000) return .{ float_bytes / 1_000.0, "KB" };
        return .{ float_bytes, "bytes" };
    }

    test formatBytes {
        try testing.expectEqual(.{ 500.0, "bytes" }, formatBytes(500));
        try testing.expectEqual(.{ 2.5, "KB" }, formatBytes(2500));
        try testing.expectEqual(.{ 5.5, "MB" }, formatBytes(5_500_000));
        try testing.expectEqual(.{ 3.7, "GB" }, formatBytes(3_700_000_000));
        try testing.expectEqual(.{ 1.0, "KB" }, formatBytes(1000));
        try testing.expectEqual(.{ 1.0, "MB" }, formatBytes(1_000_000));
        try testing.expectEqual(.{ 1.0, "GB" }, formatBytes(1_000_000_000));
    }
};

const AllocError = error{
    AllocationFailed,
    PtrStoreFailed,
};

fn allocImpl(size: usize) !*anyopaque {
    Instrumentor.once.call();
    const alignment = comptime @max(16, @alignOf(std.c.max_align_t));
    const total = size + alignment + Instrumentor.header_size;

    _ = instrumentor.total_nodes.fetchAdd(1, .acq_rel);
    _ = instrumentor.total_alloc.fetchAdd(@intCast(total), .acq_rel);
    _ = instrumentor.node_counter.fetchAdd(1, .acq_rel);

    const allocator = instrumentor.allocator();
    const mem = allocator.alloc(u8, total) catch return error.AllocationFailed;
    const base_ptr = mem.ptr;

    const aligned_ptr = blk: {
        const ptr = std.mem.alignForward(
            usize,
            @intFromPtr(base_ptr) + Instrumentor.header_size,
            alignment,
        );

        break :blk instrumentor.putKey(ptr) orelse {
            allocator.free(mem);
            return error.PtrStoreFailed;
        };
    };

    const header = @as(
        *Instrumentor.AllocHeader,
        @ptrFromInt(aligned_ptr - Instrumentor.header_size),
    );
    header.* = .{
        .size = total,
        .offset = aligned_ptr - @intFromPtr(base_ptr),
        .requested = size,
    };
    _ = instrumentor.byte_counter.fetchAdd(header.requested, .acq_rel);
    return @ptrFromInt(aligned_ptr);
}

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    return allocImpl(size) catch null;
}

const DeallocError = error{
    InvalidFree,
    HeapCorruption,
};

fn deallocImpl(ptr: ?*anyopaque) DeallocError!void {
    Instrumentor.once.call();
    const p = ptr orelse return;

    // Locks are manual here to prevent two lock/unlock cycles in one function
    instrumentor.live_lock.lock();
    defer instrumentor.live_lock.unlock();

    const key = @intFromPtr(p);
    if (!instrumentor.containsKey(key)) {
        return error.InvalidFree;
    }

    const header = blk: {
        const header = @as(
            *const Instrumentor.AllocHeader,
            @ptrFromInt(key - Instrumentor.header_size),
        );

        if (!header.valid()) {
            return error.HeapCorruption;
        }
        break :blk header;
    };

    _ = instrumentor.byte_counter.fetchSub(header.requested, .acq_rel);
    _ = instrumentor.node_counter.fetchSub(1, .acq_rel);
    _ = instrumentor.removeKey(key);

    const base_ptr = key - header.offset;
    const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
    instrumentor.allocator().free(slice);
}

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    deallocImpl(ptr) catch |err| switch (err) {
        error.InvalidFree => @panic("Double or invalid free detected"),
        error.HeapCorruption => @panic("Heap corruption detected: allocated block has malformed header"),
    };
}

test "Correct allocation pipeline" {
    for ([_]usize{ 1, 4, 16, 31, 65, 1024 }) |size| {
        const nullable_ptr = alloc(size);
        try testing.expect(nullable_ptr != null);
        defer dealloc(nullable_ptr);
        const ptr = nullable_ptr.?;

        // Verify header and alignment
        const addr = @intFromPtr(ptr);
        try testing.expectEqual(addr % 16, 0);

        const header_ptr = @as(*Instrumentor.AllocHeader, @ptrFromInt(addr - Instrumentor.header_size));
        try testing.expectEqual(Instrumentor.header_magic, header_ptr.magic);
        try testing.expectEqual(size, header_ptr.requested);
        try testing.expect(header_ptr.offset >= Instrumentor.header_size);

        // The pointer should be rw
        const user_slice = @as([*]u8, @ptrFromInt(addr))[0..size];
        @memset(user_slice, 0xAA);
        for (user_slice) |byte| {
            try testing.expectEqual(byte, 0xAA);
        }
    }
}

test "Detect double free" {
    const ptr = try allocImpl(32);
    dealloc(ptr);
    try testing.expectError(error.InvalidFree, deallocImpl(ptr));
}

test "Detect invalid free" {
    const ptr = try testing.allocator.alloc(u8, 32);
    defer testing.allocator.free(ptr);
    try testing.expectError(error.InvalidFree, deallocImpl(@ptrCast(ptr)));
}

test "Detect header corruption" {
    const ptr = try allocImpl(32);

    const addr = @intFromPtr(ptr);
    const header_ptr = @as(*Instrumentor.AllocHeader, @ptrFromInt(addr - Instrumentor.header_size));
    header_ptr.magic = 0xBADF00D;
    try testing.expectError(error.HeapCorruption, deallocImpl(ptr));

    header_ptr.magic = Instrumentor.header_magic;
    try deallocImpl(ptr);
}

test "Concurrent allocation stress" {
    var threads: [4]std.Thread = undefined;

    const Runner = struct {
        const ops_per_thread = 1000;
        const allocator = testing.allocator;

        fn run() !void {
            var ptrs: std.ArrayList(*anyopaque) = .empty;
            defer {
                for (ptrs.items) |p| {
                    dealloc(p);
                }
                ptrs.deinit(allocator);
            }

            for (0..ops_per_thread) |_| {
                try ptrs.append(allocator, try allocImpl(8));
            }
        }
    };

    for (&threads) |*t| {
        t.* = try .spawn(.{}, Runner.run, .{});
    }
    for (threads) |t| t.join();

    try testing.expectEqual(0, instrumentor.node_counter.load(.acquire));
    try testing.expectEqual(0, instrumentor.byte_counter.load(.acquire));
}
