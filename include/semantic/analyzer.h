#pragma once

#include "util/containers/array_list.h"
#include "util/memory.h"
#include "util/status.h"

typedef struct AST             AST;
typedef struct SemanticContext SemanticContext;

typedef struct SemanticAnalyzer {
    const AST*       ast;
    SemanticContext* global_ctx;
    ArrayList        errors;
    Allocator        allocator;
} SemanticAnalyzer;

NODISCARD Status seman_init(const AST* ast, SemanticAnalyzer* analyzer, Allocator allocator);
NODISCARD Status seman_null_init(SemanticAnalyzer* analyzer, Allocator allocator);
void             seman_deinit(SemanticAnalyzer* analyzer);

NODISCARD Status seman_analyze(SemanticAnalyzer* analyzer);
