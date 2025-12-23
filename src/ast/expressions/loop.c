#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/loop.h"
#include "ast/statements/block.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_for_capture_list(ArrayList* captures, free_alloc_fn free_alloc) {
    assert(captures && captures->data);
    assert(free_alloc);

    ArrayListIterator it = array_list_iterator_init(captures);
    ForLoopCapture    capture;
    while (array_list_iterator_has_next(&it, &capture)) {
        NODE_VIRTUAL_FREE(capture.capture, free_alloc);
    }

    array_list_deinit(captures);
}

NODISCARD Status for_loop_expression_create(Token               start_token,
                                            ArrayList           iterables,
                                            ArrayList           captures,
                                            BlockStatement*     block,
                                            Statement*          non_break,
                                            ForLoopExpression** for_expr,
                                            memory_alloc_fn     memory_alloc) {
    assert(memory_alloc);
    ASSERT_EXPRESSION(block);
    assert(iterables.item_size == sizeof(Expression*));
    assert(captures.item_size == sizeof(ForLoopCapture));

    ForLoopExpression* for_loop = memory_alloc(sizeof(ForLoopExpression));
    if (!for_loop) { return ALLOCATION_FAILED; }

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
    if (!node) { return; }
    assert(free_alloc);

    ForLoopExpression* for_loop = (ForLoopExpression*)node;
    free_expression_list(&for_loop->iterables, free_alloc);
    free_for_capture_list(&for_loop->captures, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->non_break, free_alloc);

    free_alloc(for_loop);
}

NODISCARD Status for_loop_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    ForLoopExpression* for_loop = (ForLoopExpression*)node;
    assert(for_loop->iterables.data && for_loop->iterables.length > 0);
    TRY(string_builder_append_str_z(sb, "for ("));

    // All for loops have an iterables clause
    ArrayListIterator it = array_list_iterator_init(&for_loop->iterables);
    Expression*       iterable;
    while (array_list_iterator_has_next(&it, &iterable)) {
        ASSERT_EXPRESSION(iterable);
        TRY(NODE_VIRTUAL_RECONSTRUCT(iterable, symbol_map, sb));
        if (!array_list_iterator_exhausted(&it)) { TRY(string_builder_append_str_z(sb, ", ")); }
    }

    // A for loop may not have captures if it means to discard its iterables
    TRY(string_builder_append(sb, ')'));
    if (for_loop->captures.length > 0) {
        TRY(string_builder_append_str_z(sb, " : ("));

        it = array_list_iterator_init(&for_loop->captures);
        ForLoopCapture capture;
        while (array_list_iterator_has_next(&it, &capture)) {
            if (capture.is_ref) { TRY(string_builder_append_str_z(sb, "ref ")); }

            ASSERT_EXPRESSION(capture.capture);
            TRY(NODE_VIRTUAL_RECONSTRUCT(capture.capture, symbol_map, sb));

            if (!array_list_iterator_exhausted(&it)) { TRY(string_builder_append_str_z(sb, ", ")); }
        }

        TRY(string_builder_append(sb, ')'));
    }

    ASSERT_STATEMENT(for_loop->block);
    TRY(string_builder_append(sb, ' '));
    TRY(NODE_VIRTUAL_RECONSTRUCT(for_loop->block, symbol_map, sb));

    // The non break clause is optional but uses an else clause like a condition
    if (for_loop->non_break) {
        ASSERT_STATEMENT(for_loop->non_break);
        TRY(string_builder_append_str_z(sb, " else "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(for_loop->non_break, symbol_map, sb));
    }

    return SUCCESS;
}

NODISCARD Status for_loop_expression_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    ForLoopExpression* for_loop = (ForLoopExpression*)node;
    assert(for_loop->iterables.data && for_loop->iterables.length > 0);
    ASSERT_STATEMENT(for_loop->block);

    MAYBE_UNUSED(for_loop);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}

NODISCARD Status while_loop_expression_create(Token                 start_token,
                                              Expression*           condition,
                                              Expression*           continuation,
                                              BlockStatement*       block,
                                              Statement*            non_break,
                                              WhileLoopExpression** while_expr,
                                              memory_alloc_fn       memory_alloc) {
    assert(memory_alloc);

    WhileLoopExpression* while_loop = memory_alloc(sizeof(WhileLoopExpression));
    if (!while_loop) { return ALLOCATION_FAILED; }

    *while_loop = (WhileLoopExpression){
        .base         = EXPRESSION_INIT(WHILE_VTABLE, start_token),
        .condition    = condition,
        .continuation = continuation,
        .block        = block,
        .non_break    = non_break,
    };

    *while_expr = while_loop;
    return SUCCESS;
}

void while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);

    WhileLoopExpression* while_loop = (WhileLoopExpression*)node;
    NODE_VIRTUAL_FREE(while_loop->condition, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->continuation, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(while_loop->non_break, free_alloc);

    free_alloc(while_loop);
}

NODISCARD Status while_loop_expression_reconstruct(Node*          node,
                                                   const HashMap* symbol_map,
                                                   StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    WhileLoopExpression* while_loop = (WhileLoopExpression*)node;
    TRY(string_builder_append_str_z(sb, "while ("));

    ASSERT_EXPRESSION(while_loop->condition);
    TRY(NODE_VIRTUAL_RECONSTRUCT(while_loop->condition, symbol_map, sb));
    TRY(string_builder_append(sb, ')'));

    // A continuation runs after every loop and is optional
    if (while_loop->continuation) {
        ASSERT_EXPRESSION(while_loop->continuation);
        TRY(string_builder_append_str_z(sb, " : ("));
        TRY(NODE_VIRTUAL_RECONSTRUCT(while_loop->continuation, symbol_map, sb));
        TRY(string_builder_append(sb, ')'));
    }

    ASSERT_STATEMENT(while_loop->block);
    TRY(string_builder_append(sb, ' '));
    TRY(NODE_VIRTUAL_RECONSTRUCT(while_loop->block, symbol_map, sb));

    // While loops also have a non break clause
    if (while_loop->non_break) {
        ASSERT_STATEMENT(while_loop->non_break);
        TRY(string_builder_append_str_z(sb, " else "));
        TRY(NODE_VIRTUAL_RECONSTRUCT(while_loop->non_break, symbol_map, sb));
    }

    return SUCCESS;
}

NODISCARD Status while_loop_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    WhileLoopExpression* while_loop = (WhileLoopExpression*)node;
    ASSERT_EXPRESSION(while_loop->condition);
    ASSERT_STATEMENT(while_loop->block);

    MAYBE_UNUSED(while_loop);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}

NODISCARD Status do_while_loop_expression_create(Token                   start_token,
                                                 BlockStatement*         block,
                                                 Expression*             condition,
                                                 DoWhileLoopExpression** do_while_expr,
                                                 memory_alloc_fn         memory_alloc) {
    assert(memory_alloc);

    DoWhileLoopExpression* do_while_loop = memory_alloc(sizeof(DoWhileLoopExpression));
    if (!do_while_loop) { return ALLOCATION_FAILED; }

    *do_while_loop = (DoWhileLoopExpression){
        .base      = EXPRESSION_INIT(DO_WHILE_VTABLE, start_token),
        .condition = condition,
        .block     = block,
    };

    *do_while_expr = do_while_loop;
    return SUCCESS;
}

void do_while_loop_expression_destroy(Node* node, free_alloc_fn free_alloc) {
    if (!node) { return; }
    assert(free_alloc);

    DoWhileLoopExpression* do_while_loop = (DoWhileLoopExpression*)node;
    NODE_VIRTUAL_FREE(do_while_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(do_while_loop->condition, free_alloc);

    free_alloc(do_while_loop);
}

NODISCARD Status do_while_loop_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb) {
    ASSERT_EXPRESSION(node);
    assert(sb);

    DoWhileLoopExpression* do_while_loop = (DoWhileLoopExpression*)node;
    TRY(string_builder_append_str_z(sb, "do "));

    ASSERT_STATEMENT(do_while_loop->block);
    TRY(NODE_VIRTUAL_RECONSTRUCT(do_while_loop->block, symbol_map, sb));

    ASSERT_EXPRESSION(do_while_loop->condition);
    TRY(string_builder_append_str_z(sb, " while ("));
    TRY(NODE_VIRTUAL_RECONSTRUCT(do_while_loop->condition, symbol_map, sb));
    TRY(string_builder_append(sb, ')'));

    return SUCCESS;
}

NODISCARD Status do_while_loop_expression_analyze(Node*            node,
                                                  SemanticContext* parent,
                                                  ArrayList*       errors) {
    ASSERT_EXPRESSION(node);
    assert(parent && errors);

    WhileLoopExpression* do_while_loop = (WhileLoopExpression*)node;
    ASSERT_STATEMENT(do_while_loop->block);
    ASSERT_EXPRESSION(do_while_loop->condition);

    MAYBE_UNUSED(do_while_loop);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
