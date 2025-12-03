#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/string.h"
#include "ast/statements/import.h"
#include "util/status.h"

TRY_STATUS
import_statement_create(Token             start_token,
                        ImportTag         tag,
                        ImportUnion       variant,
                        ImportStatement** import_stmt,
                        memory_alloc_fn   memory_alloc) {
    assert(memory_alloc);
    ImportStatement* import = memory_alloc(sizeof(ImportStatement));
    if (!import) {
        return ALLOCATION_FAILED;
    }

    *import = (ImportStatement){
        .base    = STATEMENT_INIT(IMPORT_VTABLE, start_token),
        .tag     = tag,
        .variant = variant,
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
        identifier_expression_destroy((Node*)import->variant.standard_import, free_alloc);
        break;
    case USER:
        string_literal_expression_destroy((Node*)import->variant.user_import, free_alloc);
        break;
    default:
        UNREACHABLE;
    }

    free_alloc(import);
}

TRY_STATUS import_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(string_builder_append_str_z(sb, "import "));

    ImportStatement* import = (ImportStatement*)node;
    switch (import->tag) {
    case STANDARD:
        PROPAGATE_IF_ERROR(identifier_expression_reconstruct(
            (Node*)import->variant.standard_import, symbol_map, sb));
        break;
    case USER:
        PROPAGATE_IF_ERROR(string_literal_expression_reconstruct(
            (Node*)import->variant.user_import, symbol_map, sb));
        break;
    default:
        UNREACHABLE;
    }

    PROPAGATE_IF_ERROR(string_builder_append(sb, ';'));
    return SUCCESS;
}
