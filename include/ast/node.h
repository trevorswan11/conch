#pragma once

#include "lexer/token.h"

#include "util/containers/string_builder.h"
#include "util/mem.h"
#include "util/status.h"

#define ASSERT_NODE(node) assert(node->vtable);

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

struct NodeVTable {
    void (*destroy)(Node*);
    Slice (*token_literal)(Node*);
    TRY_STATUS (*reconstruct)(Node*, StringBuilder*);
};

struct Node {
    const NodeVTable* vtable;
};
