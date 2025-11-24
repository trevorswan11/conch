#pragma once

#include "lexer/token.h"

#include "util/allocator.h"
#include "util/containers/hash_map.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

#define ASSERT_NODE(node) \
    assert(node->vtable); \
    assert(node->start_token.slice.ptr);

#define NODE_VIRTUAL_FREE(node, free_alloc)                                  \
    do {                                                                     \
        Node* _node_obfuscated = (Node*)node;                                \
        if (_node_obfuscated) {                                              \
            _node_obfuscated->vtable->destroy(_node_obfuscated, free_alloc); \
        }                                                                    \
    } while (0);

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

struct NodeVTable {
    void (*destroy)(Node*, free_alloc_fn);
    Slice (*token_literal)(Node*);
    TRY_STATUS (*reconstruct)(Node*, const HashMap*, StringBuilder*);
};

struct Node {
    const NodeVTable* vtable;
    Token             start_token;
};
