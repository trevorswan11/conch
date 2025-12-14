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

#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"

NODISCARD Status type_expression_create(Token            start_token,
                                        TypeTag          tag,
                                        TypeUnion        variant,
                                        TypeExpression** type_expr,
                                        memory_alloc_fn  memory_alloc) {
    assert(memory_alloc);
    TypeExpression* type = memory_alloc(sizeof(TypeExpression));
    if (!type) {
        return ALLOCATION_FAILED;
    }

    *type = (TypeExpression){
        .base = EXPRESSION_INIT(TYPE_VTABLE, start_token),
        .type = {tag, variant},
    };

    *type_expr = type;
    return SUCCESS;
}

void type_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);
    TypeExpression* type = (TypeExpression*)node;

    if (type->type.tag == EXPLICIT) {
        ExplicitType explicit_type = type->type.variant.explicit_type;
        switch (explicit_type.tag) {
        case EXPLICIT_IDENT: {
            IdentifierExpression* ident = explicit_type.variant.ident_type_name;
            NODE_VIRTUAL_FREE(ident, free_alloc);
            break;
        }
        case EXPLICIT_FN: {
            ExplicitFunctionType function_type = explicit_type.variant.function_type;
            free_expression_list(&function_type.fn_generics, free_alloc);
            free_parameter_list(&function_type.fn_type_params, free_alloc);
            NODE_VIRTUAL_FREE(function_type.return_type, free_alloc);
            break;
        }
        case EXPLICIT_STRUCT: {
            StructExpression* s = explicit_type.variant.struct_type;
            NODE_VIRTUAL_FREE(s, free_alloc);
            break;
        }
        case EXPLICIT_ENUM: {
            EnumExpression* e = explicit_type.variant.enum_type;
            NODE_VIRTUAL_FREE(e, free_alloc);
            break;
        }
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
    ASSERT_NODE(node);
    assert(sb);

    TypeExpression* type = (TypeExpression*)node;
    if (type->type.tag == EXPLICIT) {
        ExplicitType explicit_type = type->type.variant.explicit_type;
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
    assert(node && parent && errors);
    parent->analyzed_type.valued = false;

    Type type = ((TypeExpression*)node)->type;
    if (type.tag == IMPLICIT) {
        parent->analyzed_type.tag     = IMPLICIT_DECLARATION;
        parent->analyzed_type.variant = DATALESS_TYPE;
        return SUCCESS;
    }

    ExplicitType explicit_type     = type.variant.explicit_type;
    parent->analyzed_type.nullable = explicit_type.nullable;
    switch (explicit_type.tag) {
    case EXPLICIT_IDENT: {
        MutSlice        type_name = explicit_type.variant.ident_type_name->name;
        SemanticType    symbol_type;
        SemanticTypeTag symbol_type_tag;
        if (semantic_name_to_type_tag(type_name, &symbol_type_tag)) {
            parent->analyzed_type.tag     = symbol_type_tag;
            parent->analyzed_type.variant = DATALESS_TYPE;
        } else if (semantic_context_find(parent, true, type_name, &symbol_type)) {
            parent->analyzed_type = symbol_type;
        } else {
            const Token start_token = node->start_token;
            IGNORE_STATUS(put_status_error(
                errors, UNDECLARED_IDENTIFIER, start_token.line, start_token.column));
            return UNDECLARED_IDENTIFIER;
        }
        break;
    }
    default:
        return NOT_IMPLEMENTED;
    }

    return SUCCESS;
}

NODISCARD Status explicit_type_reconstruct(ExplicitType   explicit_type,
                                           const HashMap* symbol_map,
                                           StringBuilder* sb) {
    assert(sb);

    switch (explicit_type.tag) {
    case EXPLICIT_IDENT:
        TRY(string_builder_append_mut_slice(sb, explicit_type.variant.ident_type_name->name));
        break;
    case EXPLICIT_FN: {
        ExplicitFunctionType function_type = explicit_type.variant.function_type;
        TRY(string_builder_append_str_z(sb, "fn"));
        if (function_type.fn_generics.length > 0) {
            TRY(string_builder_append(sb, '<'));

            for (size_t i = 0; i < function_type.fn_generics.length; i++) {
                Expression* generic;
                UNREACHABLE_IF_ERROR(array_list_get(&function_type.fn_generics, i, &generic));
                Node* generic_node = (Node*)generic;
                TRY(generic_node->vtable->reconstruct(generic_node, symbol_map, sb));

                if (i != function_type.fn_generics.length - 1) {
                    TRY(string_builder_append_str_z(sb, ", "));
                }
            }

            TRY(string_builder_append(sb, '>'));
        }

        TRY(string_builder_append(sb, '('));
        TRY(reconstruct_parameter_list(&function_type.fn_type_params, symbol_map, sb));
        TRY(string_builder_append_str_z(sb, "): "));
        TRY(type_expression_reconstruct((Node*)function_type.return_type, symbol_map, sb));
        break;
    }
    case EXPLICIT_STRUCT:
        TRY(struct_expression_reconstruct(
            (Node*)explicit_type.variant.struct_type, symbol_map, sb));
        break;
    case EXPLICIT_ENUM:
        TRY(enum_expression_reconstruct((Node*)explicit_type.variant.enum_type, symbol_map, sb));
        break;
    case EXPLICIT_ARRAY:
        assert(explicit_type.variant.array_type.dimensions.length > 0);
        TRY(string_builder_append(sb, '['));

        uint64_t dim;
        for (size_t i = 0; i < explicit_type.variant.array_type.dimensions.length; i++) {
            UNREACHABLE_IF_ERROR(
                array_list_get(&explicit_type.variant.array_type.dimensions, i, &dim));
            TRY(string_builder_append_unsigned(sb, dim));

            TRY(string_builder_append(sb, 'u'));
            if (i != explicit_type.variant.array_type.dimensions.length - 1) {
                TRY(string_builder_append_str_z(sb, ", "));
            }
        }

        TRY(string_builder_append(sb, ']'));

        TypeExpression* inner_type = explicit_type.variant.array_type.inner_type;
        assert(inner_type->type.tag == EXPLICIT);

        ExplicitType inner_explicit_type = inner_type->type.variant.explicit_type;
        if (inner_explicit_type.nullable) {
            TRY(string_builder_append(sb, '?'));
        }
        TRY(explicit_type_reconstruct(inner_explicit_type, symbol_map, sb));
        break;
    }

    return SUCCESS;
}
