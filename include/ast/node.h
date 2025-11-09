#pragma once

#include "lexer/token.h"

#define MAYBE_UNUSED(x) ((void)(x))

typedef struct Node       Node;
typedef struct NodeVTable NodeVTable;

struct NodeVTable {
    const char* (*token_literal)(Node*);
    void (*destroy)(Node*);
};

struct Node {
    const NodeVTable* vtable;
};
