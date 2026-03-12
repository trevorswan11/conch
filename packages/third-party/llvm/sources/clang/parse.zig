//! https://github.com/llvm/llvm-project/blob/llvmorg-21.1.8/clang/lib/Parse/CMakeLists.txt
const LLVMBuilder = @import("../../LLVMBuilder.zig");
const SynthesizeHeaderConfig = LLVMBuilder.SynthesizeHeaderConfig;

const basic = @import("basic.zig");

pub const root = "clang/lib/Parse";
pub const sources = [_][]const u8{
    "ParseAST.cpp",
    "ParseCXXInlineMethods.cpp",
    "ParseDecl.cpp",
    "ParseDeclCXX.cpp",
    "ParseExpr.cpp",
    "ParseExprCXX.cpp",
    "ParseHLSL.cpp",
    "ParseHLSLRootSignature.cpp",
    "ParseInit.cpp",
    "ParseObjc.cpp",
    "ParseOpenMP.cpp",
    "ParsePragma.cpp",
    "ParseStmt.cpp",
    "ParseStmtAsm.cpp",
    "ParseTemplate.cpp",
    "ParseTentative.cpp",
    "Parser.cpp",
    "ParseOpenACC.cpp",
};

const attr_td = basic.attr_td;

pub const synthesize_include_configs = [_]SynthesizeHeaderConfig{
    .{
        .gen_conf = .{
            .name = "ClangAttrParserStringSwitches",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-parser-string-switches" },
        },
        .virtual_path = "clang/Parse/AttrParserStringSwitches.inc",
    },
    .{
        .gen_conf = .{
            .name = "ClangAttrSubMatchRulesParserStringSwitches",
            .td_file = attr_td,
            .instruction = .{ .action = "-gen-clang-attr-subject-match-rules-parser-string-switches" },
        },
        .virtual_path = "clang/Parse/AttrSubMatchRulesParserStringSwitches.inc",
    },
};
