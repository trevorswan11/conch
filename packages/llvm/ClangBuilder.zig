//! LLVM Source Compilation rules. All artifacts are compiled as ReleaseSafe.
const std = @import("std");

const LLVMBuilder = @import("LLVMBuilder.zig");

const Artifact = LLVMBuilder.Artifact;

const Metadata = struct {
    clang_include: std.Build.LazyPath,
};

/// Artifacts compiled for the actual target, associated with the clang subproject of llvm
const ClangTargetArtifacts = struct {
    const ClangFormat = struct {
        core_lib: Artifact = undefined,
        tool: Artifact = undefined,
    };

    basic: Artifact = undefined,
    format: ClangFormat = .{},
    rewrite: Artifact = undefined,
    tooling_core: Artifact = undefined,
};

const default_optimize = LLVMBuilder.default_optimize;
const common_llvm_cxx_flags = LLVMBuilder.common_llvm_cxx_flags;

const Self = @This();

llvm: *const LLVMBuilder,
metadata: Metadata,

clang_artifacts: ClangTargetArtifacts = .{},

/// Creates a new clang builder from a potentially unfinished LLVM
pub fn init(llvm: *const LLVMBuilder) *Self {
    const b = llvm.b;
    const self = b.allocator.create(Self) catch @panic("OOM");
    self.* = .{ .llvm = llvm, .metadata = .{
        .clang_include = llvm.metadata.root.path(b, "clang/incldue"),
    } };

    return self;
}

/// This must be called once llvm has been built with its `build`
pub fn build(self: *Self) void {
    if (!self.llvm.complete) {
        @panic("Misconfigured build script - LLVM was not built");
    }
}
