#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/statement.h"

#include "util/allocator.h"
#include "util/containers/array_list.h"
#include "util/containers/hash_set.h"
#include "util/containers/string_builder.h"
#include "util/hash.h"
#include "util/mem.h"
#include "util/status.h"

HASH_INTEGER_FN(uint32_t)
COMPARE_INTEGER_FN(int32_t)

#define CAST_FN (const void*)(uintptr_t)

static inline TRY_STATUS _init_prefix(HashSet* prefix_set, Allocator allocator) {
    PROPAGATE_IF_ERROR(hash_set_init_allocator(prefix_set,
                                               64,
                                               sizeof(PrefixFn),
                                               alignof(PrefixFn),
                                               hash_prefix,
                                               compare_prefix,
                                               allocator));
    const size_t num_prefix = sizeof(PREFIX_FUNCTIONS) / sizeof(PREFIX_FUNCTIONS[0]);
    for (size_t i = 0; i < num_prefix; i++) {
        const PrefixFn fn = PREFIX_FUNCTIONS[i];
        hash_set_put_assume_capacity(prefix_set, &fn);
    }

    return SUCCESS;
}

static inline TRY_STATUS _init_infix(HashSet* infix_set, Allocator allocator) {
    PROPAGATE_IF_ERROR(hash_set_init_allocator(
        infix_set, 64, sizeof(InfixFn), alignof(InfixFn), hash_infix, compare_infix, allocator));

    // TODO
    // const size_t num_infix = sizeof(INFIX_FUNCTIONS) / sizeof(INFIX_FUNCTIONS[0]);
    // for (size_t i = 0; i < num_infix; i++) {
    //     const InfixFn fn = INFIX_FUNCTIONS[i];
    //     hash_set_put_assume_capacity(infix_set, &fn);
    // }

    return SUCCESS;
}

TRY_STATUS parser_init(Parser* p, Lexer* l, FileIO* io, Allocator allocator) {
    if (!p || !l || !io) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(allocator);

    HashSet prefix_functions;
    PROPAGATE_IF_ERROR(_init_prefix(&prefix_functions, allocator));

    HashSet infix_functions;
    PROPAGATE_IF_ERROR_DO(_init_infix(&infix_functions, allocator),
                          hash_set_deinit(&prefix_functions));

    ArrayList errors;
    PROPAGATE_IF_ERROR_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
    });

    *p = (Parser){
        .lexer            = l,
        .lexer_index      = 0,
        .current_token    = token_init(END, "", 0, 0, 0),
        .peek_token       = token_init(END, "", 0, 0, 0),
        .prefix_parse_fns = prefix_functions,
        .infix_parse_fns  = infix_functions,
        .errors           = errors,
        .io               = io,
        .allocator        = allocator,
    };

    // Read twice to set current and peek
    PROPAGATE_IF_ERROR(parser_next_token(p));
    PROPAGATE_IF_ERROR(parser_next_token(p));
    return SUCCESS;
}

void parser_deinit(Parser* p) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    MutSlice allocated;
    for (size_t i = 0; i < p->errors.length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(&p->errors, i, &allocated));
        p->allocator.free_alloc(allocated.ptr);
    }

    array_list_deinit(&p->errors);
    hash_set_deinit(&p->prefix_parse_fns);
    hash_set_deinit(&p->infix_parse_fns);
}

TRY_STATUS parser_consume(Parser* p, AST* ast) {
    if (!p || !p->lexer || !p->io || !ast) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(ast->allocator);

    p->lexer_index   = 0;
    p->current_token = token_init(END, "", 0, 0, 0);
    p->peek_token    = token_init(END, "", 0, 0, 0);

    PROPAGATE_IF_ERROR(parser_next_token(p));
    PROPAGATE_IF_ERROR(parser_next_token(p));

    array_list_clear_retaining_capacity(&ast->statements);
    array_list_clear_retaining_capacity(&p->errors);

    // Traverse the tokens and append until exhausted
    Statement* stmt = NULL;
    while (!parser_current_token_is(p, END)) {
        PROPAGATE_IF_ERROR_IS(parser_parse_statement(p, &stmt), ALLOCATION_FAILED);
        if (stmt) {
            PROPAGATE_IF_ERROR_DO(array_list_push(&ast->statements, &stmt), {
                Node* node = (Node*)stmt;
                node->vtable->destroy(node, p->allocator.free_alloc);
            });
        }
        IGNORE_STATUS(parser_next_token(p));
    }

    // If we encountered any errors, invalidate the tree for now
    if (p->errors.length > 0) {
        array_list_clear_retaining_capacity(&ast->statements);
    }

    return SUCCESS;
}

TRY_STATUS parser_next_token(Parser* p) {
    p->current_token = p->peek_token;
    return array_list_get(&p->lexer->token_accumulator, p->lexer_index++, &p->peek_token);
}

bool parser_current_token_is(const Parser* p, TokenType t) {
    assert(p);
    return p->current_token.type == t;
}

bool parser_peek_token_is(const Parser* p, TokenType t) {
    assert(p);
    return p->peek_token.type == t;
}

TRY_STATUS parser_expect_peek(Parser* p, TokenType t) {
    assert(p);
    if (parser_peek_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        PROPAGATE_IF_ERROR_IS(parser_peek_error(p, t), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }
}

#define PPE_CLEANUP string_builder_deinit(&builder)

TRY_STATUS parser_peek_error(Parser* p, TokenType t) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    StringBuilder builder;
    PROPAGATE_IF_ERROR(string_builder_init_allocator(&builder, 60, p->allocator));

    // Genreal token information
    const char start[] = "Expected token ";
    const char mid[]   = ", found ";

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, start, sizeof(start) - 1),
                          PPE_CLEANUP);

    const char* expected = token_type_name(t);
    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, expected, strlen(expected)),
                          PPE_CLEANUP);

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, mid, sizeof(mid) - 1), PPE_CLEANUP);

    const char* actual = token_type_name(p->peek_token.type);
    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, actual, strlen(actual)),
                          PPE_CLEANUP);

    // Append line/col information for debugging
    const char line_no[] = " [Ln ";
    const char col_no[]  = ", Col ";

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, line_no, sizeof(line_no) - 1),
                          PPE_CLEANUP);

    PROPAGATE_IF_ERROR_DO(string_builder_append_size(&builder, p->peek_token.line), PPE_CLEANUP);

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, col_no, sizeof(col_no) - 1),
                          PPE_CLEANUP);

    PROPAGATE_IF_ERROR_DO(string_builder_append_size(&builder, p->peek_token.column), PPE_CLEANUP);

    PROPAGATE_IF_ERROR_DO(string_builder_append(&builder, ']'), PPE_CLEANUP);

    MutSlice slice;
    PROPAGATE_IF_ERROR_DO(string_builder_to_string(&builder, &slice), PPE_CLEANUP);
    PROPAGATE_IF_ERROR_DO(array_list_push(&p->errors, &slice), PPE_CLEANUP);
    return SUCCESS;
}

TRY_STATUS parser_parse_statement(Parser* p, Statement** stmt) {
    switch (p->current_token.type) {
    case VAR:
    case CONST:
        PROPAGATE_IF_ERROR(decl_statement_parse(p, (DeclStatement**)stmt));
        break;
    case RETURN:
        PROPAGATE_IF_ERROR(return_statement_parse(p, (ReturnStatement**)stmt));
        break;
    default:
        PROPAGATE_IF_ERROR(expression_statement_parse(p, (ExpressionStatement**)stmt));
        break;
    }

    return SUCCESS;
}
