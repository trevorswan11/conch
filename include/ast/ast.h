#pragma once

#include "util/containers/array_list.h"
#include "util/error.h"

typedef struct {
    ArrayList statements;
} AST;

AnyError ast_init(AST* ast);
void     ast_deinit(AST* ast);
