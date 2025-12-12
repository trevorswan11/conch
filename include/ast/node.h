#pragma once

#include "lexer/token.h"

#include "util/allocator.h"
#include "util/status.h"

#define ASSERT_NODE(node) \
    assert(node->vtable); \
    assert(node->start_token.slice.ptr);

// Casts the input pointer to a node, calls is virtual destructor, and nulls the pointer
#define NODE_VIRTUAL_FREE(node_ptr_to_cast_and_free, free_alloc)                     \
    do {                                                                             \
        Node* _node_obfuscated_nvf = (Node*)node_ptr_to_cast_and_free;               \
        if (node_ptr_to_cast_and_free && _node_obfuscated_nvf) {                     \
            _node_obfuscated_nvf->vtable->destroy(_node_obfuscated_nvf, free_alloc); \
        }                                                                            \
        node_ptr_to_cast_and_free = NULL;                                            \
    } while (0);

// Casts the input pointer to a node and calls its virtual analyzer
#define NODE_VIRTUAL_ANALYZE(node_ptr_to_cast, context, errors) \
    ((Node*)node_ptr_to_cast)->vtable->analyze((Node*)node_ptr_to_cast, context, errors)

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

typedef struct HashMap       HashMap;
typedef struct StringBuilder StringBuilder;

typedef struct ArrayList       ArrayList;
typedef struct SemanticContext SemanticContext;

struct NodeVTable {
    void (*destroy)(Node*, free_alloc_fn);
    NODISCARD Status (*reconstruct)(Node*, const HashMap*, StringBuilder*);
    NODISCARD Status (*analyze)(Node*, SemanticContext*, ArrayList*);
};

struct Node {
    const NodeVTable* vtable;
    Token             start_token;
};
