#ifndef LOOP_EXPR_H
#define LOOP_EXPR_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

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

[[nodiscard]] Status for_loop_expression_create(Token               start_token,
                                            ArrayList           iterables,
                                            ArrayList           captures,
                                            BlockStatement*     block,
                                            Statement*          non_break,
                                            ForLoopExpression** for_expr,
                                            memory_alloc_fn     memory_alloc);

void             for_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status for_loop_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb);
[[nodiscard]] Status for_loop_expression_analyze(Node*            node,
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

[[nodiscard]] Status while_loop_expression_create(Token                 start_token,
                                              Expression*           condition,
                                              Expression*           continuation,
                                              BlockStatement*       block,
                                              Statement*            non_break,
                                              WhileLoopExpression** while_expr,
                                              memory_alloc_fn       memory_alloc);

void             while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status while_loop_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb);
[[nodiscard]] Status while_loop_expression_analyze(Node*            node,
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

[[nodiscard]] Status do_while_loop_expression_create(Token                   start_token,
                                                 BlockStatement*         block,
                                                 Expression*             condition,
                                                 DoWhileLoopExpression** do_while_expr,
                                                 memory_alloc_fn         memory_alloc);

void             do_while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status do_while_loop_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb);
[[nodiscard]] Status do_while_loop_expression_analyze(Node*            node,
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

typedef struct LoopExpression {
    Expression      base;
    BlockStatement* block;
} LoopExpression;

[[nodiscard]] Status loop_expression_create(Token            start_token,
                                        BlockStatement*  block,
                                        LoopExpression** loop_expr,
                                        memory_alloc_fn  memory_alloc);

void             loop_expression_destroy(Node* node, free_alloc_fn free_alloc);
[[nodiscard]] Status loop_expression_reconstruct(Node*          node,
                                             const HashMap* symbol_map,
                                             StringBuilder* sb);
[[nodiscard]] Status loop_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable LOOP_VTABLE = {
    .base =
        {
            .destroy     = loop_expression_destroy,
            .reconstruct = loop_expression_reconstruct,
            .analyze     = loop_expression_analyze,
        },
};

#endif
