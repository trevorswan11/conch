#include <assert.h>

#include "ast/expressions/single.h"

#include "semantic/context.h"
#include "semantic/type.h"

#include "util/containers/array_list.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

#define SINGLE_STMT_CREATE(type, custom_vtab, out_expr)            \
    assert(memory_alloc);                                          \
    type* temp = memory_alloc(sizeof(type));                       \
    if (!temp) {                                                   \
        return ALLOCATION_FAILED;                                  \
    }                                                              \
                                                                   \
    *temp     = (type){EXPRESSION_INIT(custom_vtab, start_token)}; \
    *out_expr = temp;                                              \
    return SUCCESS

#define SINGLE_STMT_RECONSTRUCT(string)           \
    assert(sb);                                   \
    MAYBE_UNUSED(node);                           \
    MAYBE_UNUSED(symbol_map);                     \
                                                  \
    TRY(string_builder_append_str_z(sb, string)); \
    return SUCCESS

void single_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    free_alloc(node);
}

NODISCARD Status nil_expression_create(Token           start_token,
                                       NilExpression** nil_expr,
                                       memory_alloc_fn memory_alloc) {
    SINGLE_STMT_CREATE(NilExpression, NIL_VTABLE, nil_expr);
}

NODISCARD Status nil_expression_reconstruct(Node*          node,
                                            const HashMap* symbol_map,
                                            StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("nil");
}

NODISCARD Status nil_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(errors);

    parent->analyzed_type.tag      = NIL_VALUE;
    parent->analyzed_type.variant  = DATALESS_TYPE;
    parent->analyzed_type.valued   = true;
    parent->analyzed_type.nullable = true;
    return SUCCESS;
}

NODISCARD Status ignore_expression_create(Token              start_token,
                                          IgnoreExpression** ignore_expr,
                                          memory_alloc_fn    memory_alloc) {
    SINGLE_STMT_CREATE(IgnoreExpression, IGNORE_VTABLE, ignore_expr);
}

NODISCARD Status ignore_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    SINGLE_STMT_RECONSTRUCT("_");
}

NODISCARD Status ignore_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return SUCCESS;
}
