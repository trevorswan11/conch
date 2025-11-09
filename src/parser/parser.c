#include <assert.h>
#include <stdbool.h>

#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "lexer/lexer.h"
#include "lexer/token.h"

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"
#include "util/containers/string_builder.h"
#include "util/mem.h"

bool parser_init(Parser* p, Lexer* l, FileIO* io) {
    if (!p || !l) {
        return false;
    }

    ArrayList errors;
    if (!array_list_init(&errors, 10, sizeof(MutSlice))) {
        return false;
    }

    *p = (Parser){
        .lexer         = l,
        .lexer_index   = 0,
        .current_token = token_init(END, "", 0),
        .peek_token    = token_init(END, "", 0),
        .errors        = errors,
        .io            = io,
    };

    // Read twice to set current and peek
    if (!parser_next_token(p) || !parser_next_token(p)) {
        return false;
    }
    return true;
}

void parser_deinit(Parser* p) {
    MutSlice allocated;
    for (size_t i = 0; i < p->errors.length; i++) {
        array_list_get(&p->errors, i, &allocated);
        free(allocated.ptr);
    }

    array_list_deinit(&p->errors);
}

bool parser_consume(Parser* p, AST* ast) {
    if (!p || !p->lexer || !ast) {
        return false;
    }

    p->lexer_index   = 0;
    p->current_token = token_init(END, "", 0);
    p->peek_token    = token_init(END, "", 0);

    if (!parser_next_token(p) || !parser_next_token(p)) {
        return false;
    }

    array_list_clear_retaining_capacity(&ast->statements);
    array_list_clear_retaining_capacity(&p->errors);

    // Traverse the tokens and append until exhausted
    while (!parser_current_token_is(p, END)) {
        Statement* stmt = parser_parse_statement(p);
        if (stmt) {
            if (!array_list_push(&ast->statements, &stmt)) {
                return false;
            }
        }
        parser_next_token(p);
    }

    return true;
}

bool parser_next_token(Parser* p) {
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

bool parser_expect_peek(Parser* p, TokenType t) {
    assert(p);
    if (parser_peek_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        parser_peek_error(p, t);
        return false;
    }
}

bool parser_peek_error(Parser* p, TokenType t) {
    assert(p);

    StringBuilder builder;
    string_builder_init(&builder, 60);

    const char start[] = "Expected token ";
    const char mid[]   = ", found ";

    if (!string_builder_append_many(&builder, start, sizeof(start) - 1)) {
        string_builder_deinit(&builder);
        return false;
    }

    const char* expected = token_type_name(t);
    if (!string_builder_append_many(&builder, expected, strlen(expected))) {
        string_builder_deinit(&builder);
        return false;
    }

    if (!string_builder_append_many(&builder, mid, sizeof(mid) - 1)) {
        string_builder_deinit(&builder);
        return false;
    }

    const char* actual = token_type_name(p->peek_token.type);
    if (!string_builder_append_many(&builder, actual, strlen(actual))) {
        string_builder_deinit(&builder);
        return false;
    }

    if (!string_builder_append(&builder, '.')) {
        string_builder_deinit(&builder);
        return false;
    }

    MutSlice slice = string_builder_to_string(&builder);
    if (!array_list_push(&p->errors, &slice)) {
        string_builder_deinit(&builder);
        return false;
    }
    return true;
}

Statement* parser_parse_statement(Parser* p) {
    switch (p->current_token.type) {
    case VAR: {
        VarStatement* var_stmt = var_statement_parse(p);
        return (Statement*)var_stmt;
    }
    default:
        return NULL;
    }
}
