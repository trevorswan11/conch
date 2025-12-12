#pragma once

typedef enum {
    SIGNED_INTEGER,
    UNSIGNED_INTEGER,
    FLOATING_POINT,
} SemanticTypeTag;

typedef struct PrimitiveType {
    char _;
} PrimitiveType;

typedef union {
    PrimitiveType primitive_type;
} SemanticTypeUnion;

static const SemanticTypeUnion PRIMITIVE_TYPE = {.primitive_type = {'\0'}};

typedef struct SemanticType {
    SemanticTypeTag   tag;
    SemanticTypeUnion variant;
} SemanticType;
