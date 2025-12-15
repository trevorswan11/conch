#include <assert.h>

#include "ast/ast.h"
#include "ast/expressions/loop.h"
#include "ast/statements/block.h"

#include "semantic/context.h"

#include "util/containers/string_builder.h"

void free_for_capture_list(ArrayList* captures, free_alloc_fn free_alloc) {
    assert(captures && captures->data);
    assert(free_alloc);

    ForLoopCapture capture;
    for (size_t i = 0; i < captures->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(captures, i, &capture));
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
    assert(iterables.item_size == sizeof(Expression*));
    assert(captures.item_size == sizeof(ForLoopCapture));

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
    free_for_capture_list(&for_loop->captures, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->block, free_alloc);
    NODE_VIRTUAL_FREE(for_loop->non_break, free_alloc);

    free_alloc(for_loop);
}

NODISCARD Status for_loop_expression_reconstruct(Node*          node,
                                                 const HashMap* symbol_map,
                                                 StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    ForLoopExpression* for_loop = (ForLoopExpression*)node;
    TRY(string_builder_append_str_z(sb, "for ("));

    // All for loops have an iterables clause
    for (size_t i = 0; i < for_loop->iterables.length; i++) {
        Expression* iterable;
        UNREACHABLE_IF_ERROR(array_list_get(&for_loop->iterables, i, &iterable));
        Node* iter_node = (Node*)iterable;
        TRY(iter_node->vtable->reconstruct(iter_node, symbol_map, sb));

        if (i != for_loop->iterables.length - 1) {
            TRY(string_builder_append_str_z(sb, ", "));
        }
    }

    // A for loop may not have captures if it means to discard its iterables
    TRY(string_builder_append(sb, ')'));
    if (for_loop->captures.length > 0) {
        TRY(string_builder_append_str_z(sb, " : ("));

        for (size_t i = 0; i < for_loop->captures.length; i++) {
            ForLoopCapture capture;
            UNREACHABLE_IF_ERROR(array_list_get(&for_loop->captures, i, &capture));
            if (capture.is_ref) {
                TRY(string_builder_append_str_z(sb, "ref "));
            }

            Node* capture_node = (Node*)capture.capture;
            TRY(capture_node->vtable->reconstruct(capture_node, symbol_map, sb));

            if (i != for_loop->captures.length - 1) {
                TRY(string_builder_append_str_z(sb, ", "));
            }
        }

        TRY(string_builder_append(sb, ')'));
    }

    TRY(string_builder_append(sb, ' '));
    TRY(block_statement_reconstruct((Node*)for_loop->block, symbol_map, sb));

    // The non break clause is optional but uses an else clause like a condition
    if (for_loop->non_break) {
        TRY(string_builder_append_str_z(sb, " else "));
        Node* non_break = (Node*)for_loop->non_break;
        TRY(non_break->vtable->reconstruct(non_break, symbol_map, sb));
    }

    return SUCCESS;
}

NODISCARD Status for_loop_expression_analyze(Node*            node,
                                             SemanticContext* parent,
                                             ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
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
    if (!while_loop) {
        return ALLOCATION_FAILED;
    }

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
    ASSERT_NODE(node);
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
    ASSERT_NODE(node);
    assert(sb);

    WhileLoopExpression* while_loop = (WhileLoopExpression*)node;
    TRY(string_builder_append_str_z(sb, "while ("));

    Node* cond_node = (Node*)while_loop->condition;
    TRY(cond_node->vtable->reconstruct(cond_node, symbol_map, sb));
    TRY(string_builder_append(sb, ')'));

    // A continuation runs after every loop and is optional
    if (while_loop->continuation) {
        TRY(string_builder_append_str_z(sb, " : ("));
        Node* cont_node = (Node*)while_loop->continuation;
        TRY(cont_node->vtable->reconstruct(cont_node, symbol_map, sb));
        TRY(string_builder_append(sb, ')'));
    }

    TRY(string_builder_append(sb, ' '));
    TRY(block_statement_reconstruct((Node*)while_loop->block, symbol_map, sb));

    // While loops also have a non break clause
    if (while_loop->non_break) {
        TRY(string_builder_append_str_z(sb, " else "));
        Node* non_break = (Node*)while_loop->non_break;
        TRY(non_break->vtable->reconstruct(non_break, symbol_map, sb));
    }

    return SUCCESS;
}

NODISCARD Status while_loop_expression_analyze(Node*            node,
                                               SemanticContext* parent,
                                               ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
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
    if (!do_while_loop) {
        return ALLOCATION_FAILED;
    }

    *do_while_loop = (DoWhileLoopExpression){
        .base      = EXPRESSION_INIT(DO_WHILE_VTABLE, start_token),
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

NODISCARD Status do_while_loop_expression_reconstruct(Node*          node,
                                                      const HashMap* symbol_map,
                                                      StringBuilder* sb) {
    ASSERT_NODE(node);
    assert(sb);

    DoWhileLoopExpression* do_while_loop = (DoWhileLoopExpression*)node;
    TRY(string_builder_append_str_z(sb, "do "));
    TRY(block_statement_reconstruct((Node*)do_while_loop->block, symbol_map, sb));

    TRY(string_builder_append_str_z(sb, " while ("));
    Node* cond_node = (Node*)do_while_loop->condition;
    TRY(cond_node->vtable->reconstruct(cond_node, symbol_map, sb));
    TRY(string_builder_append(sb, ')'));

    return SUCCESS;
}

NODISCARD Status do_while_loop_expression_analyze(Node*            node,
                                                  SemanticContext* parent,
                                                  ArrayList*       errors) {
    assert(node && parent && errors);
    MAYBE_UNUSED(node);
    MAYBE_UNUSED(parent);
    MAYBE_UNUSED(errors);
    return NOT_IMPLEMENTED;
}
