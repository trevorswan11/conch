const std = @import("std");

const c = @cImport({
    @cInclude("archive.h");
    @cInclude("archive_entry.h");
});

pub fn main() !void {
    var arena: std.heap.ArenaAllocator = .init(std.heap.c_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    var stderr_buf: [1024]u8 = undefined;
    const stderr_handle = std.fs.File.stderr();
    var writer = stderr_handle.writer(&stderr_buf);
    const stderr = &writer.interface;

    const args = try std.process.argsAlloc(allocator);
    if (args.len < 4) {
        try stderr.print("Usage: compressor [tar|zst] [output_file] [input_dir]\n", .{});
        try stderr.flush();
        return error.UsageError;
    }

    const format = args[1];
    const out_path = args[2];
    const in_dir = args[3];

    const archive = c.archive_write_new() orelse return error.ArchiveNewFailed;
    defer _ = c.archive_write_free(archive);

    // Zip and zstd archives are supported
    if (std.mem.eql(u8, format, "zip")) {
        _ = c.archive_write_set_format_zip(archive);
    } else if (std.mem.eql(u8, format, "zst")) {
        _ = c.archive_write_set_format_pax_restricted(archive);
        _ = c.archive_write_add_filter_zstd(archive);
    } else {
        return error.InvalidArchiveFormat;
    }

    if (c.archive_write_open_filename(archive, out_path.ptr) != c.ARCHIVE_OK) {
        try stderr.print("Open Error: {s}\n", .{c.archive_error_string(archive)});
        try stderr.flush();
        return error.ArchiveOpenFailed;
    }
    defer _ = c.archive_write_close(archive);

    // The build system guarantees that everything in the input directory is ready to be compressed
    var dir = try std.fs.cwd().openDir(in_dir, .{ .iterate = true });
    defer dir.close();
    var walker = try dir.walk(allocator);
    defer walker.deinit();

    const buf_size = 64 * 1024;
    var file_reader_buf: [buf_size]u8 = undefined;
    const input_buf = file_reader_buf[0 .. buf_size / 2];
    const output_buf = file_reader_buf[buf_size / 2 ..];

    while (try walker.next()) |entry| {
        if (entry.kind != .file) continue;
        const archive_entry = c.archive_entry_new() orelse return error.EntryNewFailed;
        defer c.archive_entry_free(archive_entry);
        c.archive_entry_set_pathname(archive_entry, entry.path.ptr);

        // File stats for the header
        const stat = try entry.dir.statFile(entry.basename);
        c.archive_entry_set_size(archive_entry, @intCast(stat.size));

        // https://github.com/libarchive/libarchive/blob/master/libarchive/archive_entry.h#L216
        c.archive_entry_set_filetype(archive_entry, 0o100000);
        c.archive_entry_set_perm(archive_entry, 0o644);

        if (c.archive_write_header(archive, archive_entry) != c.ARCHIVE_OK) {
            try stderr.print("Header error: {s}\n", .{c.archive_error_string(archive)});
            try stderr.flush();
            continue;
        }

        // Stream the file data into the archive
        const file = try entry.dir.openFile(entry.basename, .{});
        defer file.close();
        var reader = file.reader(input_buf);

        while (!reader.atEnd()) {
            const bytes_read = try reader.interface.readSliceShort(output_buf);
            _ = c.archive_write_data(archive, output_buf, bytes_read);
        }
    }

    try stderr.flush();
}
