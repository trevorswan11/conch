#include <assert.h>

#include "ast/expressions/identifier.h"
#include "ast/statements/import.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

[[nodiscard]] Status import_statement_create(Token                 start_token,
                                             ImportTag             tag,
                                             ImportUnion           variant,
                                             IdentifierExpression* alias,
                                             ImportStatement**     import_stmt,
                                             Allocator*            allocator) {
    ASSERT_ALLOCATOR_PTR(allocator);
    ImportStatement* import = ALLOCATOR_PTR_MALLOC(allocator, sizeof(*import));
    if (!import) { return ALLOCATION_FAILED; }

    *import = (ImportStatement){
        .base    = STATEMENT_INIT(IMPORT_VTABLE, start_token),
        .tag     = tag,
        .variant = variant,
        .alias   = alias,
    };

    *import_stmt = import;
    return SUCCESS;
}

void import_statement_destroy(Node* node, Allocator* allocator) {
    if (!node) { return; }
    ASSERT_ALLOCATOR_PTR(allocator);

    ImportStatement* import = (ImportStatement*)node;
    switch (import->tag) {
    case STANDARD:
        NODE_VIRTUAL_FREE(import->variant.standard_import, allocator);
        break;
    case USER:
        NODE_VIRTUAL_FREE(import->variant.user_import, allocator);
        break;
    }

    NODE_VIRTUAL_FREE(import->alias, allocator);
    ALLOCATOR_PTR_FREE(allocator, import);
}

[[nodiscard]] Status
import_statement_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_STATEMENT(node);
    assert(sb);

    TRY(string_builder_append_str_z(sb, "import "));

    ImportStatement* import = (ImportStatement*)node;
    switch (import->tag) {
    case STANDARD:
        ASSERT_EXPRESSION(import->variant.standard_import);
        TRY(NODE_VIRTUAL_RECONSTRUCT(import->variant.standard_import, symbol_map, sb));
        break;
    case USER:
        ASSERT_EXPRESSION(import->variant.user_import);
        TRY(NODE_VIRTUAL_RECONSTRUCT(import->variant.user_import, symbol_map, sb));
        break;
    default:
        UNREACHABLE;
    }

    if (import->alias) {
        ASSERT_EXPRESSION(import->alias);
        TRY(string_builder_append_str_z(sb, " as "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(import->alias, symbol_map, sb));
    }

    TRY(string_builder_append(sb, ';'));
    return SUCCESS;
}

[[nodiscard]] Status import_statement_analyze(Node*                             node,
                                              [[maybe_unused]] SemanticContext* parent,
                                              [[maybe_unused]] ArrayList*       errors) {
    ASSERT_STATEMENT(node);
    assert(parent && errors);

    [[maybe_unused]] ImportStatement* import = (ImportStatement*)node;

    return NOT_IMPLEMENTED;
}
