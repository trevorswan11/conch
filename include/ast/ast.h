#ifndef AST_H
#define AST_H

#include "lexer/token.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct StringBuilder StringBuilder;

// A global flag that determines how prefix and infix expressions are printed.
//
// When true, parenthesis are generously added to reconstructions.
// This can be useful for verifying operator precedence, but is disabled by default.
extern bool group_expressions;

void clear_error_list(ArrayList* errors, Allocator* allocator);
void free_error_list(ArrayList* errors, Allocator* allocator);

typedef struct AST {
    ArrayList statements;
    HashMap   token_type_symbols;
    Allocator allocator;
} AST;

[[nodiscard]] Status ast_init(AST* ast, Allocator* allocator);
void                 ast_deinit(AST* ast);

// Gets a pointer to the AST's allocator.
// The AST is asserted to be non-null here.
//
// The returned allocator is guaranteed to have a valid vtable.
Allocator* ast_allocator(AST* ast);

// Reconstructs the original source code from the AST.
//
// The builder is cleared on entry, and freed on error.
[[nodiscard]] Status ast_reconstruct(AST* ast, StringBuilder* sb);

// Tries to get the symbol for the given token type.
//
// If the symbol does not exist, returns the standard representation.
Slice poll_tt_symbol(const HashMap* symbol_map, TokenType t);

void clear_statement_list(ArrayList* statements, Allocator* allocator);
void clear_expression_list(ArrayList* expressions, Allocator* allocator);

void free_statement_list(ArrayList* statements, Allocator* allocator);
void free_expression_list(ArrayList* expressions, Allocator* allocator);

// Handles the reconstruction of generic lists.
//
// While the generics pointer must be valid, this is a no-op for empty lists.
[[nodiscard]] Status
generics_reconstruct(ArrayList* generics, const HashMap* symbol_map, StringBuilder* sb);

#endif
