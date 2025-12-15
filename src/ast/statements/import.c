#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/string.h"
#include "ast/statements/import.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

NODISCARD Status import_statement_create(Token                 start_token,
                                         ImportTag             tag,
                                         ImportUnion           variant,
                                         IdentifierExpression* alias,
                                         ImportStatement**     import_stmt,
                                         memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);
    ImportStatement* import = memory_alloc(sizeof(ImportStatement));
    if (!import) {
        return ALLOCATION_FAILED;
    }

    *import = (ImportStatement){
        .base    = STATEMENT_INIT(IMPORT_VTABLE, start_token),
        .tag     = tag,
        .variant = variant,
        .alias   = alias,
    };

    *import_stmt = import;
    return SUCCESS;
}

void import_statement_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ImportStatement* import = (ImportStatement*)node;
    switch (import->tag) {
    case STANDARD:
        NODE_VIRTUAL_FREE(import->variant.standard_import, free_alloc);
        break;
    case USER:
        NODE_VIRTUAL_FREE(import->variant.user_import, free_alloc);
        break;
    }

    NODE_VIRTUAL_FREE(import->alias, free_alloc);
    free_alloc(import);
}

NODISCARD Status import_statement_reconstruct(Node*          node,
                                              const HashMap* symbol_map,
                                              StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "import "));

    ImportStatement* import = (ImportStatement*)node;
    switch (import->tag) {
    case STANDARD:
        TRY(identifier_expression_reconstruct(
            (Node*)import->variant.standard_import, symbol_map, sb));
        break;
    case USER:
        TRY(string_literal_expression_reconstruct(
            (Node*)import->variant.user_import, symbol_map, sb));
        break;
    default:
        UNREACHABLE;
    }

    if (import->alias) {
        TRY(string_builder_append_str_z(sb, " as "));
        TRY(identifier_expression_reconstruct((Node*)import->alias, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

NODISCARD Status import_statement_analyze(Node* node, SemanticContext* parent, ArrayList* errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
