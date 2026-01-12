const std = @import("std");

extern fn launch(argc: i32, argv: [*]const [:0]const u8) i32;

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

    const header_magic = 0xdeadbeef;
    const header_size = @sizeOf(AllocHeader);

    gpa: std.heap.GeneralPurposeAllocator(.{
        .thread_safe = true,
    }),

    process_args: ?[][:0]u8 = null,

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
        if (self.process_args) |args| {
            std.process.argsFree(internal_allocator, args);
        }
    }

    pub fn manageProcessArgs(self: *Instrumentor) ![]const [:0]u8 {
        self.process_args = try std.process.argsAlloc(Instrumentor.internal_allocator);
        return self.process_args.?;
    }

    pub fn allocator(self: *Instrumentor) std.mem.Allocator {
        return self.gpa.allocator();
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

    pub fn tryDumpLeaks(self: *Instrumentor) bool {
        return self.gpa.detectLeaks();
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

    const args = try instrumentor.manageProcessArgs();
    const proc = launch(@intCast(args.len), args.ptr);

    const result: u8 = @intCast(@intFromBool(instrumentor.tryDumpLeaks()) | proc);
    instrumentor.report();
    std.process.exit(result);
}

export fn alloc(size: usize) callconv(.c) ?*anyopaque {
    instrumentor_once.call();
    const alignment = comptime @max(16, @alignOf(std.c.max_align_t));
    const total = size + alignment + Instrumentor.header_size;

    _ = instrumentor.total_nodes.fetchAdd(1, .acq_rel);
    _ = instrumentor.total_alloc.fetchAdd(@intCast(total), .acq_rel);
    _ = instrumentor.node_counter.fetchAdd(1, .acq_rel);

    const mem = instrumentor.allocator().alloc(u8, total) catch return null;
    const base_ptr = mem.ptr;

    const aligned_ptr = blk: {
        const ptr = std.mem.alignForward(
            usize,
            @intFromPtr(base_ptr) + Instrumentor.header_size,
            alignment,
        );

        break :blk instrumentor.putKey(ptr) orelse return null;
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

export fn dealloc(ptr: ?*anyopaque) callconv(.c) void {
    instrumentor_once.call();
    const p = ptr orelse return;
    if (!instrumentor.removeKey(@intFromPtr(p))) {
        @panic("Double or invalid free detected");
    }

    const header = blk: {
        const header = @as(
            *const Instrumentor.AllocHeader,
            @ptrFromInt(@intFromPtr(p) - Instrumentor.header_size),
        );

        if (!header.valid()) {
            @panic("Heap corruption detected: allocated block has malformed header");
        }
        break :blk header;
    };

    _ = instrumentor.byte_counter.fetchSub(header.requested, .acq_rel);
    _ = instrumentor.node_counter.fetchSub(1, .acq_rel);

    const base_ptr = @intFromPtr(p) - header.offset;
    const slice = @as([*]u8, @ptrFromInt(base_ptr))[0..header.size];
    instrumentor.allocator().free(slice);
}
