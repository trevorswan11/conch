#pragma once

#include "lexer/token.h"

#include "ast/expressions/expression.h"
#include "ast/node.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/status.h"

typedef struct BlockStatement BlockStatement;

typedef struct ForLoopCapture {
    bool        is_ref;
    Expression* capture;
} ForLoopCapture;

typedef struct ForLoopExpression {
    Expression      base;
    ArrayList       iterables;
    ArrayList       captures;
    BlockStatement* block;
    Statement*      non_break;
} ForLoopExpression;

void free_for_capture_list(ArrayList* captures, free_alloc_fn free_alloc);

NODISCARD Status for_loop_expression_create(Token               start_token,
                                            ArrayList           iterables,
                                            ArrayList           captures,
                                            BlockStatement*     block,
                                            Statement*          non_break,
                                            ForLoopExpression** for_expr,
                                            memory_alloc_fn     memory_alloc);

void             for_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status for_loop_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb);
NODISCARD Status for_loop_expression_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors);

static const ExpressionVTable FOR_VTABLE = {
    .base =
        {
            .destroy     = for_loop_expression_destroy,
            .reconstruct = for_loop_expression_reconstruct,
            .analyze     = for_loop_expression_analyze,
        },
};

typedef struct WhileLoopExpression {
    Expression      base;
    Expression*     condition;
    Expression*     continuation;
    BlockStatement* block;
    Statement*      non_break;
} WhileLoopExpression;

NODISCARD Status while_loop_expression_create(Token                 start_token,
                                              Expression*           condition,
                                              Expression*           continuation,
                                              BlockStatement*       block,
                                              Statement*            non_break,
                                              WhileLoopExpression** while_expr,
                                              memory_alloc_fn       memory_alloc);

void             while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status while_loop_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb);
NODISCARD Status while_loop_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors);

static const ExpressionVTable WHILE_VTABLE = {
    .base =
        {
            .destroy     = while_loop_expression_destroy,
            .reconstruct = while_loop_expression_reconstruct,
            .analyze     = while_loop_expression_analyze,
        },
};

typedef struct DoWhileLoopExpression {
    Expression      base;
    BlockStatement* block;
    Expression*     condition;
} DoWhileLoopExpression;

NODISCARD Status do_while_loop_expression_create(Token                   start_token,
                                                 BlockStatement*         block,
                                                 Expression*             condition,
                                                 DoWhileLoopExpression** do_while_expr,
                                                 memory_alloc_fn         memory_alloc);

void             do_while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
NODISCARD Status do_while_loop_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb);
NODISCARD Status do_while_loop_expression_analyze(Node*            node,
                                                  SemanticContext* parent,
                                                  ArrayList*       errors);

static const ExpressionVTable DO_WHILE_VTABLE = {
    .base =
        {
            .destroy     = do_while_loop_expression_destroy,
            .reconstruct = do_while_loop_expression_reconstruct,
            .analyze     = do_while_loop_expression_analyze,
        },
};
