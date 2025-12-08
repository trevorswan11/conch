#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/expressions/string.h"
#include "ast/statements/import.h"
#include "util/status.h"

NODISCARD Status import_statement_create(Token             start_token,
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
        NODE_VIRTUAL_FREE(import->variant.standard_import, free_alloc);
        break;
    case USER:
        NODE_VIRTUAL_FREE(import->variant.user_import, free_alloc);
        break;
    }

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

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}
