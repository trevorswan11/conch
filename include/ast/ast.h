#pragma once

#include "lexer/token.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

typedef struct {
    ArrayList statements;
    HashMap   token_type_symbols;
    Allocator allocator;
} AST;

TRY_STATUS ast_init(AST* ast, Allocator allocator);
void       ast_deinit(AST* ast);

// Reconstructs the original source code from the AST.
//
// The builder is cleared on entry, and freed on error.
TRY_STATUS ast_reconstruct(AST* ast, StringBuilder* sb);

// Tries to get the symbol for the given token type.
//
// If the symbol does not exist, returns the standard representation.
Slice poll_tt_symbol(const HashMap* symbol_map, TokenType t);

void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc);
void clear_expression_list(ArrayList* expressions, free_alloc_fn free_alloc);
