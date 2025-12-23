#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_struct_member_list(ArrayList* members, free_alloc_fn free_alloc) {
    assert(members);
    assert(free_alloc);

    ArrayListIterator it = array_list_iterator_init(members);
    StructMember      member;
    while (array_list_iterator_has_next(&it, &member)) {
        NODE_VIRTUAL_FREE(member.name, free_alloc);
        NODE_VIRTUAL_FREE(member.type, free_alloc);
        NODE_VIRTUAL_FREE(member.default_value, free_alloc);
    }
    array_list_deinit(members);
}

NODISCARD Status struct_expression_create(Token              start_token,
                                          ArrayList          generics,
                                          ArrayList          members,
                                          StructExpression** struct_expr,
                                          memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);
    assert(generics.item_size = sizeof(Expression*));
    assert(members.length > 0);
    assert(members.item_size = sizeof(StructMember));

    StructExpression* struct_local = memory_alloc(sizeof(StructExpression));
    if (!struct_local) {
        return ALLOCATION_FAILED;
    }

    *struct_local = (StructExpression){
        .base     = EXPRESSION_INIT(STRUCT_VTABLE, start_token),
        .generics = generics,
        .members  = members,
    };

    *struct_expr = struct_local;
    return SUCCESS;
}

void struct_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);

    StructExpression* struct_expr = (StructExpression*)node;
    free_expression_list(&struct_expr->generics, free_alloc);
    free_struct_member_list(&struct_expr->members, free_alloc);

    free_alloc(struct_expr);
}

NODISCARD Status struct_expression_reconstruct(Node*          node,
                                               const HashMap* symbol_map,
                                               StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    StructExpression* struct_expr = (StructExpression*)node;
    TRY(string_builder_append_str_z(sb, "struct"));

    // Struct generics introduce slightly different spacing
    TRY(generics_reconstruct(&struct_expr->generics, symbol_map, sb));
    if (struct_expr->generics.length == 0) {
        TRY(string_builder_append(sb, ' '));
    }
    TRY(string_builder_append_str_z(sb, "{ "));

    ArrayListIterator it = array_list_iterator_init(&struct_expr->members);
    StructMember      member;
    while (array_list_iterator_has_next(&it, &member)) {
        ASSERT_EXPRESSION(member.name);
        TRY(NODE_VIRTUAL_RECONSTRUCT(member.name, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, ": "));
        ASSERT_EXPRESSION(member.type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(member.type, symbol_map, sb));

        if (member.default_value) {
            ASSERT_EXPRESSION(member.default_value);
            TRY(string_builder_append_str_z(sb, " = "));
            TRY(NODE_VIRTUAL_RECONSTRUCT(member.default_value, symbol_map, sb));
        }
        TRY(string_builder_append_str_z(sb, ", "));
    }

    TRY(string_builder_append(sb, '}'));
    return SUCCESS;
}

NODISCARD Status struct_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    StructExpression* struct_expr = (StructExpression*)node;
    assert(struct_expr->members.data && struct_expr->members.length > 0);

    MAYBE_UNUSED(struct_expr);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
