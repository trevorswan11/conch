#pragma once

#include "lexer/token.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

#define ASSERT_NODE(node) assert(node->vtable);
#define NODE_VIRTUAL_FREE(node, free_alloc)                      \
    if (node) {                                                  \
        ((Node*)node)->vtable->destroy((Node*)node, free_alloc); \
    }

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

struct NodeVTable {
    void (*destroy)(Node*, free_alloc_fn);
    Slice (*token_literal)(Node*);
    TRY_STATUS (*reconstruct)(Node*, const HashMap*, StringBuilder*);
};

struct Node {
    const NodeVTable* vtable;
};
