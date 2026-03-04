const std = @import("std");

pub const Config = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

upstream: *std.Build.Dependency,
artifact: *std.Build.Step.Compile,
