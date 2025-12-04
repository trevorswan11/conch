#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/loop.h"
#include "ast/statements/block.h"

TRY_STATUS for_loop_expression_create(Token               start_token,
                                      ArrayList           iterables,
                                      ArrayList           captures,
                                      BlockStatement*     block,
                                      Statement*          non_break,
                                      ForLoopExpression** for_expr,
                                      memory_alloc_fn     memory_alloc) {
    assert(memory_alloc);
    assert(iterables.item_size == sizeof(Expression*));
    assert(captures.item_size == sizeof(Expression*));

    if (iterables.length == 0) {
        return FOR_MISSING_ITERABLES;
    } else if (!block) {
        return LOOP_MISSING_BODY;
    }

    ForLoopExpression* for_loop = memory_alloc(sizeof(ForLoopExpression));
    if (!for_loop) {
        return ALLOCATION_FAILED;
    }

    *for_loop = (ForLoopExpression){
        .base      = EXPRESSION_INIT(FOR_VTABLE, start_token),
        .iterables = iterables,
        .captures  = captures,
        .block     = block,
        .non_break = non_break,
    };

    *for_expr = for_loop;
    return SUCCESS;
}

void for_loop_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    ForLoopExpression* for_loop = (ForLoopExpression*)node;
    free_expression_list(&for_loop->iterables, free_alloc);
    free_expression_list(&for_loop->captures, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->non_break, free_alloc);

    free_alloc(for_loop);
}

TRY_STATUS
for_loop_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    MAYBE_UNUSED(node);
    MAYBE_UNUSED(symbol_map);
    return NOT_IMPLEMENTED;
}

TRY_STATUS while_loop_expression_create(Token                 start_token,
                                        Expression*           condition,
                                        Expression*           continuation,
                                        BlockStatement*       block,
                                        Statement*            non_break,
                                        WhileLoopExpression** while_expr,
                                        memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);

    if (!condition) {
        return WHILE_MISSING_CONDITION;
    } else if (!block) {
        return LOOP_MISSING_BODY;
    }

    WhileLoopExpression* while_loop = memory_alloc(sizeof(WhileLoopExpression));
    if (!while_loop) {
        return ALLOCATION_FAILED;
    }

    *while_loop = (WhileLoopExpression){
        .base         = EXPRESSION_INIT(FOR_VTABLE, start_token),
        .condition    = condition,
        .continuation = continuation,
        .block        = block,
        .non_break    = non_break,
    };

    *while_expr = while_loop;
    return SUCCESS;
}

void while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    WhileLoopExpression* while_loop = (WhileLoopExpression*)node;
    NODE_VIRTUAL_FREE(while_loop->condition, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->continuation, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->non_break, free_alloc);

    free_alloc(while_loop);
}

TRY_STATUS
while_loop_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    MAYBE_UNUSED(node);
    MAYBE_UNUSED(symbol_map);
    return NOT_IMPLEMENTED;
}

TRY_STATUS do_while_loop_expression_create(Token                   start_token,
                                           BlockStatement*         block,
                                           Expression*             condition,
                                           DoWhileLoopExpression** do_while_expr,
                                           memory_alloc_fn         memory_alloc) {
    assert(memory_alloc);

    if (!block) {
        return LOOP_MISSING_BODY;
    } else if (!condition) {
        return WHILE_MISSING_CONDITION;
    }

    DoWhileLoopExpression* do_while_loop = memory_alloc(sizeof(DoWhileLoopExpression));
    if (!do_while_loop) {
        return ALLOCATION_FAILED;
    }

    *do_while_loop = (DoWhileLoopExpression){
        .base      = EXPRESSION_INIT(FOR_VTABLE, start_token),
        .condition = condition,
        .block     = block,
    };

    *do_while_expr = do_while_loop;
    return SUCCESS;
}

void do_while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    ASSERT_NODE(node);
    assert(free_alloc);

    DoWhileLoopExpression* do_while_loop = (DoWhileLoopExpression*)node;
    NODE_VIRTUAL_FREE(do_while_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(do_while_loop->condition, free_alloc);

    free_alloc(do_while_loop);
}

TRY_STATUS
do_while_loop_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb) {
    ASSERT_NODE(node);
    if (!sb) {
        return NULL_PARAMETER;
    }

    MAYBE_UNUSED(node);
    MAYBE_UNUSED(symbol_map);
    return NOT_IMPLEMENTED;
}
