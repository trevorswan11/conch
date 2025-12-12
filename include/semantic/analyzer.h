#pragma once

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/status.h"

typedef struct AST             AST;
typedef struct SemanticContext SemanticContext;

typedef struct SemanticAnalyzer {
    AST*             ast;
    SemanticContext* global_ctx;
    ArrayList        errors;
} SemanticAnalyzer;

NODISCARD Status seman_init(AST* ast, SemanticAnalyzer* analyzer, Allocator allocator);
void             seman_deinit(SemanticAnalyzer* analyzer, free_alloc_fn free_alloc);

NODISCARD Status seman_analyze(SemanticAnalyzer* analyzer);
