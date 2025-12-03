#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"

void free_struct_member_list(ArrayList* members, free_alloc_fn free_alloc) {
    assert(members);
    assert(free_alloc);

    StructMember member;
    for (size_t i = 0; i < members->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(members, i, &member));
        identifier_expression_destroy((Node*)member.name, free_alloc);
        type_expression_destroy((Node*)member.type, free_alloc);
        NODE_VIRTUAL_FREE(member.default_value, free_alloc);
    }
    array_list_deinit(members);
}

TRY_STATUS struct_expression_create(Token              start_token,
                                    ArrayList          members,
                                    StructExpression** struct_expr,
                                    memory_alloc_fn    memory_alloc) {
    assert(memory_alloc);
    assert(start_token.slice.ptr);

    if (members.length == 0) {
        return STRUCT_MISSING_MEMBERS;
    }

    StructExpression* struct_local = memory_alloc(sizeof(StructExpression));
    if (!struct_local) {
        return ALLOCATION_FAILED;
    }

    *struct_local = (StructExpression){
        .base    = EXPRESSION_INIT(STRUCT_VTABLE, start_token),
        .members = members,
    };

    *struct_expr = struct_local;
    return SUCCESS;
}

void struct_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    StructExpression* struct_expr = (StructExpression*)node;
    free_struct_member_list(&struct_expr->members, free_alloc);
    free_alloc(struct_expr);
}

TRY_STATUS
struct_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    StructExpression* s = (StructExpression*)node;
    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "struct { "));

    StructMember member;
    for (size_t i = 0; i < s->members.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&s->members, i, &member));

        Node* member_name = (Node*)member.name;
        PROPAGATE_IF_ERROR(member_name->vtable->reconstruct(member_name, symbol_map, sb));

        Node* member_type = (Node*)member.type;
        PROPAGATE_IF_ERROR(member_type->vtable->reconstruct(member_type, symbol_map, sb));

        if (member.default_value) {
            Node* member_default = (Node*)member.default_value;
            PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, " = "));
            PROPAGATE_IF_ERROR(member_default->vtable->reconstruct(member_default, symbol_map, sb));
        }
        PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, ", "));
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, '}'));
    return SUCCESS;
}
