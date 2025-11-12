#pragma once

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/status.h"

typedef struct {
    ArrayList statements;
} AST;

TRY_STATUS ast_init(AST* ast);
void       ast_deinit(AST* ast);

// Reconstructs the original source code from the AST.
//
// The builder is cleared on entry, and freed on error.
TRY_STATUS ast_reconstruct(AST* ast, StringBuilder* sb);
