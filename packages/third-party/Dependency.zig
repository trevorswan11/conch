const std = @import("std");

pub const Config = struct {
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
};

pub const Artifact = *std.Build.Step.Compile;

upstream: *std.Build.Dependency,
artifact: *std.Build.Step.Compile,
