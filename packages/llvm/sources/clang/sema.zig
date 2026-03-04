//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Sema/CMakeLists.txt
const std = @import("std");

const LLVMBuilder = @import("../../LLVMBuilder.zig");
const SynthesizeHeaderConfig = LLVMBuilder.SynthesizeHeaderConfig;

const basic = @import("basic.zig");

pub const include_root = "clang/include/clang/Sema";
pub const attr_td = basic.attr_td;

pub const attr_synthesize_configs = [_]SynthesizeHeaderConfig{
    .{
        .gen_conf = .{
            .name = "ClangAttrTemplateInstantiate",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-template-instantiate" },
        },
        .virtual_path = "clang/Sema/AttrTemplateInstantiate.inc",
    },
    .{
        .gen_conf = .{
            .name = "ClangAttrParsedAttrKinds",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-parsed-attr-kinds" },
        },
        .virtual_path = "clang/Sema/AttrParsedAttrKinds.inc",
    },
    .{
        .gen_conf = .{
            .name = "ClangAttrSpellingListIndex",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-spelling-index" },
        },
        .virtual_path = "clang/Sema/AttrSpellingListIndex.inc",
    },
    .{
        .gen_conf = .{
            .name = "ClangAttrParsedAttrImpl",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-parsed-attr-impl" },
        },
        .virtual_path = "clang/Sema/AttrParsedAttrImpl.inc",
    },
};
