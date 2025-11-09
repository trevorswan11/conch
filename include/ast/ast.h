#pragma once

#include "util/containers/array_list.h"

typedef struct {
    ArrayList statements;
} AST;

bool ast_init(AST* ast);
void ast_deinit(AST* ast);
