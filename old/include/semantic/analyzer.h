#ifndef SEMA_ANALYZER_H
#define SEMA_ANALYZER_H

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

[[nodiscard]] Status generics_analyze(const ArrayList*       generic_exprs,
                                      const SemanticContext* context,
                                      ArrayList*             generics,
                                      Allocator*             allocator);
void                 analyzed_generics_deinit(ArrayList* generics, Allocator* allocator);

[[nodiscard]] Status seman_init(const AST* ast, SemanticAnalyzer* analyzer, Allocator* allocator);
[[nodiscard]] Status seman_null_init(SemanticAnalyzer* analyzer, Allocator* allocator);
void                 seman_deinit(SemanticAnalyzer* analyzer);

[[nodiscard]] Status seman_analyze(SemanticAnalyzer* analyzer);

#endif
