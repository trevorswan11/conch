#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/enum.h"
#include "ast/expressions/function.h"
#include "ast/expressions/identifier.h"
#include "ast/expressions/struct.h"
#include "ast/expressions/type.h"

#include "semantic/context.h"
#include "semantic/symbol.h"
#include "semantic/type.h"

#include "util/containers/string_builder.h"

NODISCARD Status type_expression_create(Token               start_token,
                                        TypeExpressionTag   tag,
                                        TypeExpressionUnion variant,
                                        TypeExpression**    type_expr,
                                        memory_alloc_fn     memory_alloc) {
    assert(memory_alloc);
    TypeExpression* type = memory_alloc(sizeof(TypeExpression));
    if (!type) {
        return ALLOCATION_FAILED;
    }

    *type = (TypeExpression){
        .base    = EXPRESSION_INIT(TYPE_VTABLE, start_token),
        .tag     = tag,
        .variant = variant,
    };

    *type_expr = type;
    return SUCCESS;
}

void type_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) {
        return;
    }
    assert(free_alloc);
    TypeExpression* type = (TypeExpression*)node;

    if (type->tag == EXPLICIT) {
        ExplicitType explicit_type = type->variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT:
            NODE_VIRTUAL_FREE(explicit_type.variant.ident_type_name, free_alloc);
            break;
        case EXPLICIT_TYPEOF:
            NODE_VIRTUAL_FREE(explicit_type.variant.referred_type, free_alloc);
            break;
        case EXPLICIT_FN: {
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            free_expression_list(&function_type.generics, free_alloc);
            free_parameter_list(&function_type.parameters, free_alloc);
            NODE_VIRTUAL_FREE(function_type.return_type, free_alloc);
            break;
        }
        case EXPLICIT_STRUCT:
            NODE_VIRTUAL_FREE(explicit_type.variant.struct_type, free_alloc);
            break;
        case EXPLICIT_ENUM:
            NODE_VIRTUAL_FREE(explicit_type.variant.enum_type, free_alloc);
            break;
        case EXPLICIT_ARRAY: {
            TypeExpression* inner = explicit_type.variant.array_type.inner_type;
            array_list_deinit(&explicit_type.variant.array_type.dimensions);
            NODE_VIRTUAL_FREE(inner, free_alloc);
            break;
        }
        }
    }

    free_alloc(type);
}

NODISCARD Status type_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    TypeExpression* type = (TypeExpression*)node;
    if (type->tag == EXPLICIT) {
        ExplicitType explicit_type = type->variant.explicit_type;
        if (explicit_type.nullable) {
            TRY(string_builder_append(sb, '?'));
        }

        TRY(explicit_type_reconstruct(explicit_type, symbol_map, sb));
    } else {
        TRY(string_builder_append_str_z(sb, " :"));
    }
    return SUCCESS;
}

NODISCARD Status type_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    const Token     start_token = node->start_token;
    const Allocator allocator   = parent->symbol_table->symbols.allocator;

    TypeExpression* type = (TypeExpression*)node;
    SemanticType*   new_symbol_type;

    if (type->tag == IMPLICIT) {
        TRY(semantic_type_create(&new_symbol_type, allocator.memory_alloc));
        new_symbol_type->tag      = STYPE_IMPLICIT_DECLARATION;
        new_symbol_type->variant  = SEMANTIC_DATALESS_TYPE;
        new_symbol_type->is_const = true;
        new_symbol_type->valued   = false;
        new_symbol_type->nullable = false;
    } else {
        const ExplicitType explicit_type = type->variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT: {
            TRY(semantic_type_create(&new_symbol_type, allocator.memory_alloc));
            new_symbol_type->is_const = true;
            new_symbol_type->valued   = false;

            MutSlice      probe_type_name = explicit_type.variant.ident_type_name->name;
            SemanticType* probe_symbol_type;

            SemanticTypeTag symbol_type_tag;
            if (semantic_name_to_primitive_type_tag(probe_type_name, &symbol_type_tag)) {
                new_symbol_type->tag      = symbol_type_tag;
                new_symbol_type->variant  = SEMANTIC_DATALESS_TYPE;
                new_symbol_type->nullable = explicit_type.nullable;
            } else if (semantic_context_find(parent, true, probe_type_name, &probe_symbol_type)) {
                // Double null doesn't make sense, guard before making the new type
                if (probe_symbol_type->nullable && explicit_type.nullable) {
                    PUT_STATUS_PROPAGATE(errors,
                                         DOUBLE_NULLABLE,
                                         start_token,
                                         RC_RELEASE(new_symbol_type, allocator.free_alloc));
                }

                // Underlying copy ignores nullable, so inherit from either source
                TRY_DO(semantic_type_copy_variant(new_symbol_type, probe_symbol_type, allocator),
                       RC_RELEASE(new_symbol_type, allocator.free_alloc));
                new_symbol_type->nullable = probe_symbol_type->nullable || explicit_type.nullable;
            } else {
                PUT_STATUS_PROPAGATE(errors,
                                     UNDECLARED_IDENTIFIER,
                                     start_token,
                                     RC_RELEASE(new_symbol_type, allocator.free_alloc));
            }
            break;
        }
        case EXPLICIT_TYPEOF: {
            TRY(semantic_type_create(&new_symbol_type, allocator.memory_alloc));

            // We need to copy into since we may be changing nullable status
            const Expression* referred = explicit_type.variant.referred_type;
            TRY_DO(NODE_VIRTUAL_ANALYZE(referred, parent, errors),
                   RC_RELEASE(new_symbol_type, allocator.free_alloc));
            SemanticType* analyzed = semantic_context_move_analyzed(parent);

            TRY_DO(semantic_type_copy_variant(new_symbol_type, analyzed, allocator), {
                RC_RELEASE(analyzed, allocator.free_alloc);
                RC_RELEASE(new_symbol_type, allocator.free_alloc);
            });

            RC_RELEASE(analyzed, allocator.free_alloc);
            new_symbol_type->nullable = explicit_type.nullable;
            break;
        }
        case EXPLICIT_ENUM: {
            // The enum is brand new lifting and we can safely alter nullable
            const EnumExpression* enum_expr = explicit_type.variant.enum_type;
            TRY(NODE_VIRTUAL_ANALYZE(enum_expr, parent, errors));

            new_symbol_type           = semantic_context_move_analyzed(parent);
            new_symbol_type->nullable = explicit_type.nullable;
            break;
        }
        default:
            return NOT_IMPLEMENTED;
        }

        assert(new_symbol_type->is_const);
        assert(!new_symbol_type->valued);
    }

    parent->analyzed_type = new_symbol_type;
    return SUCCESS;
}

NODISCARD Status explicit_type_reconstruct(ExplicitType   explicit_type,
                                           const HashMap* symbol_map,
                                           StringBuilder* sb) {
    assert(sb);

    switch (explicit_type.tag) {
    case EXPLICIT_IDENT:
        ASSERT_EXPRESSION(explicit_type.variant.ident_type_name);
        TRY(string_builder_append_mut_slice(sb, explicit_type.variant.ident_type_name->name));
        break;
    case EXPLICIT_TYPEOF:
        ASSERT_EXPRESSION(explicit_type.variant.referred_type);
        TRY(string_builder_append_str_z(sb, "typeof "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.referred_type, symbol_map, sb));
        break;
    case EXPLICIT_FN: {
        ExplicitFunctionType function_type = explicit_type.variant.function_type;
        TRY(string_builder_append_str_z(sb, "fn"));
        if (function_type.generics.length > 0) {
            TRY(string_builder_append(sb, '<'));

            ArrayListIterator it = array_list_iterator_init(&function_type.generics);
            Expression*       generic;
            while (array_list_iterator_has_next(&it, &generic)) {
                ASSERT_EXPRESSION(generic);
                TRY(NODE_VIRTUAL_RECONSTRUCT(generic, symbol_map, sb));
                if (!array_list_iterator_exhausted(&it)) {
                    TRY(string_builder_append_str_z(sb, ", "));
                }
            }

            TRY(string_builder_append(sb, '>'));
        }

        TRY(string_builder_append(sb, '('));
        TRY(reconstruct_parameter_list(&function_type.parameters, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, "): "));
        ASSERT_EXPRESSION(function_type.return_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(function_type.return_type, symbol_map, sb));
        break;
    }
    case EXPLICIT_STRUCT:
        ASSERT_EXPRESSION(explicit_type.variant.struct_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.struct_type, symbol_map, sb));
        break;
    case EXPLICIT_ENUM:
        ASSERT_EXPRESSION(explicit_type.variant.enum_type);
        TRY(NODE_VIRTUAL_RECONSTRUCT(explicit_type.variant.enum_type, symbol_map, sb));
        break;
    case EXPLICIT_ARRAY: {
        assert(explicit_type.variant.array_type.dimensions.length > 0);
        TRY(string_builder_append(sb, '['));

        ArrayListIterator it =
            array_list_iterator_init(&explicit_type.variant.array_type.dimensions);
        uint64_t dim;
        while (array_list_iterator_has_next(&it, &dim)) {
            TRY(string_builder_append_unsigned(sb, dim));
            TRY(string_builder_append(sb, 'u'));

            if (!array_list_iterator_exhausted(&it)) {
                TRY(string_builder_append_str_z(sb, ", "));
            }
        }

        TRY(string_builder_append(sb, ']'));

        TypeExpression* inner_type = explicit_type.variant.array_type.inner_type;
        assert(inner_type->tag == EXPLICIT);

        ExplicitType inner_explicit_type = inner_type->variant.explicit_type;
        if (inner_explicit_type.nullable) {
            TRY(string_builder_append(sb, '?'));
        }
        TRY(explicit_type_reconstruct(inner_explicit_type, symbol_map, sb));
        break;
    }
    }

    return SUCCESS;
}
