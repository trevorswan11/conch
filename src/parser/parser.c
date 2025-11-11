#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/error.h"
#include "util/mem.h"

AnyError parser_init(Parser* p, Lexer* l, FileIO* io) {
    if (!p || !l || !io) {
        return NULL_PARAMETER;
    }

    ArrayList errors;
    PROPAGATE_IF_ERROR(array_list_init(&errors, 10, sizeof(MutSlice)));

    *p = (Parser){
        .lexer         = l,
        .lexer_index   = 0,
        .current_token = token_init(END, "", 0, 0, 0),
        .peek_token    = token_init(END, "", 0, 0, 0),
        .errors        = errors,
        .io            = io,
    };

    // Read twice to set current and peek
    PROPAGATE_IF_ERROR(parser_next_token(p));
    PROPAGATE_IF_ERROR(parser_next_token(p));
    return SUCCESS;
}

void parser_deinit(Parser* p) {
    MutSlice allocated;
    for (size_t i = 0; i < p->errors.length; i++) {
        array_list_get(&p->errors, i, &allocated);
        free(allocated.ptr);
    }

    array_list_deinit(&p->errors);
}

AnyError parser_consume(Parser* p, AST* ast) {
    if (!p || !p->lexer || !p->io || !ast) {
        return NULL_PARAMETER;
    }

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
            PROPAGATE_IF_ERROR(array_list_push(&ast->statements, &stmt));
        }
        parser_next_token(p);
    }

    // If we encountered any errors, invalidate the tree for now
    if (p->errors.length > 0) {
        array_list_clear_retaining_capacity(&ast->statements);
    }

    return SUCCESS;
}

AnyError parser_next_token(Parser* p) {
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

AnyError parser_expect_peek(Parser* p, TokenType t) {
    assert(p);
    if (parser_peek_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        PROPAGATE_IF_ERROR_IS(parser_peek_error(p, t), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }
}

#define PPE_CLEANUP string_builder_deinit(&builder)

AnyError parser_peek_error(Parser* p, TokenType t) {
    assert(p);

    StringBuilder builder;
    string_builder_init(&builder, 60);

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

AnyError parser_parse_statement(Parser* p, Statement** stmt) {
    switch (p->current_token.type) {
    case VAR:
        PROPAGATE_IF_ERROR(decl_statement_parse(p, false, (DeclStatement**)stmt));
        break;
    case CONST:
        PROPAGATE_IF_ERROR(decl_statement_parse(p, true, (DeclStatement**)stmt));
        break;
    case RETURN:
        PROPAGATE_IF_ERROR(return_statement_parse(p, (ReturnStatement**)stmt));
        break;
    default:
        return UNEXPECTED_TOKEN;
    }

    return SUCCESS;
}
