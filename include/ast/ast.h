#pragma once

#include <stdbool.h>

#include "lexer/token.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

// A global flag that determines how prefix and infix expressions are printed.
//
// When true, parenthesis are generously added to reconstructions.
// This can be useful for verifying operator precedence, but is disabled by default.
extern bool group_expressions;

typedef struct AST {
    ArrayList statements;
    HashMap   token_type_symbols;
    Allocator allocator;
} AST;

NODISCARD Status ast_init(AST* ast, Allocator allocator);
void             ast_deinit(AST* ast);

// Reconstructs the original source code from the AST.
//
// The builder is cleared on entry, and freed on error.
NODISCARD Status ast_reconstruct(AST* ast, StringBuilder* sb);

// Tries to get the symbol for the given token type.
//
// If the symbol does not exist, returns the standard representation.
Slice poll_tt_symbol(const HashMap* symbol_map, TokenType t);

void clear_statement_list(ArrayList* statements, free_alloc_fn free_alloc);
void clear_expression_list(ArrayList* expressions, free_alloc_fn free_alloc);

void free_statement_list(ArrayList* statements, free_alloc_fn free_alloc);
void free_expression_list(ArrayList* expressions, free_alloc_fn free_alloc);

// Handles the reconstruction of generic lists.
//
// While the generics pointer must be valid, this is a no-op for empty lists.
NODISCARD Status generics_reconstruct(ArrayList*     generics,
                                      const HashMap* symbol_map,
                                      StringBuilder* sb);
