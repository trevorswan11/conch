#ifndef NODE_H
#define NODE_H

#include "lexer/token.h"

#include "util/memory.h"
#include "util/status.h"

#define ASSERT_NODE(node)          \
    assert(node);                  \
    assert(((Node*)node)->vtable); \
    assert(((Node*)node)->start_token.slice.ptr);

// Casts the input pointer to a node, calls is virtual destructor, and nulls the pointer
#define NODE_VIRTUAL_FREE(node_ptr_to_cast_and_free, free_alloc)                   \
    do {                                                                           \
        Node* node_obfuscated_nvf = (Node*)node_ptr_to_cast_and_free;              \
        if (node_ptr_to_cast_and_free && node_obfuscated_nvf) {                    \
            node_obfuscated_nvf->vtable->destroy(node_obfuscated_nvf, free_alloc); \
        }                                                                          \
        node_ptr_to_cast_and_free = nullptr;                                       \
    } while (0);

// Casts the input pointer to a node and calls its virtual reconstruct function
#define NODE_VIRTUAL_RECONSTRUCT(node_ptr_to_cast, symbol_map, sb_ptr) \
    ((Node*)node_ptr_to_cast)->vtable->reconstruct((Node*)node_ptr_to_cast, symbol_map, sb_ptr)

// Casts the input pointer to a node and calls its virtual analyzer
#define NODE_VIRTUAL_ANALYZE(node_ptr_to_cast, context, errors) \
    ((Node*)node_ptr_to_cast)->vtable->analyze((Node*)node_ptr_to_cast, context, errors)

// Casts the input pointer to a node and retrieves its start token
#define NODE_TOKEN(node_ptr_to_cast) ((Node*)node_ptr_to_cast)->start_token

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

typedef struct HashMap       HashMap;
typedef struct StringBuilder StringBuilder;

typedef struct ArrayList       ArrayList;
typedef struct SemanticContext SemanticContext;

struct NodeVTable {
    void (*destroy)(Node*, free_alloc_fn);
    Status (*reconstruct)(Node*, const HashMap*, StringBuilder*);
    Status (*analyze)(Node*, SemanticContext*, ArrayList*);
};

struct Node {
    const NodeVTable* vtable;
    Token             start_token;
};

#endif
