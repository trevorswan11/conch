#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "ast/expressions/type.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/jump.h"

#include "util/containers/string_builder.h"

static inline TRY_STATUS _init_prefix(HashSet* prefix_set, Allocator allocator) {
    const size_t num_prefix = sizeof(PREFIX_FUNCTIONS) / sizeof(PREFIX_FUNCTIONS[0]);
    PROPAGATE_IF_ERROR(hash_set_init_allocator(prefix_set,
                                               num_prefix,
                                               sizeof(PrefixFn),
                                               alignof(PrefixFn),
                                               hash_prefix,
                                               compare_prefix,
                                               allocator));

    for (size_t i = 0; i < num_prefix; i++) {
        const PrefixFn fn = PREFIX_FUNCTIONS[i];
        hash_set_put_assume_capacity(prefix_set, &fn);
    }

    return SUCCESS;
}

static inline TRY_STATUS _init_infix(HashSet* infix_set, Allocator allocator) {
    const size_t num_infix = sizeof(INFIX_FUNCTIONS) / sizeof(INFIX_FUNCTIONS[0]);
    const size_t num_pairs = sizeof(PRECEDENCE_PAIRS) / sizeof(PRECEDENCE_PAIRS[0]);
    if (num_infix < num_pairs) {
        return VIOLATED_INVARIANT;
    }

    PROPAGATE_IF_ERROR(hash_set_init_allocator(infix_set,
                                               num_infix,
                                               sizeof(InfixFn),
                                               alignof(InfixFn),
                                               hash_infix,
                                               compare_infix,
                                               allocator));

    for (size_t i = 0; i < num_infix; i++) {
        const InfixFn fn = INFIX_FUNCTIONS[i];
        hash_set_put_assume_capacity(infix_set, &fn);
    }

    return SUCCESS;
}

static inline TRY_STATUS _init_primitives(HashSet* primitive_set, Allocator allocator) {
    const size_t num_primitives = sizeof(ALL_PRIMITIVES) / sizeof(ALL_PRIMITIVES[0]);
    PROPAGATE_IF_ERROR(hash_set_init_allocator(primitive_set,
                                               num_primitives,
                                               sizeof(TokenType),
                                               alignof(TokenType),
                                               hash_token_type,
                                               compare_token_type,
                                               allocator));

    for (size_t i = 0; i < num_primitives; i++) {
        const Keyword keyword = ALL_PRIMITIVES[i];
        hash_set_put_assume_capacity(primitive_set, &keyword.type);
    }

    return SUCCESS;
}

static inline TRY_STATUS _init_precedences(HashMap* precedence_map, Allocator allocator) {
    const size_t num_pairs = sizeof(PRECEDENCE_PAIRS) / sizeof(PRECEDENCE_PAIRS[0]);
    PROPAGATE_IF_ERROR(hash_map_init_allocator(precedence_map,
                                               num_pairs,
                                               sizeof(TokenType),
                                               alignof(TokenType),
                                               sizeof(Precedence),
                                               alignof(Precedence),
                                               hash_token_type,
                                               compare_token_type,
                                               allocator));

    for (size_t i = 0; i < num_pairs; i++) {
        const PrecedencePair pair = PRECEDENCE_PAIRS[i];
        hash_map_put_assume_capacity(precedence_map, &pair.token_key, &pair.precedence);
    }

    return SUCCESS;
}

void clear_error_list(ArrayList* errors, free_alloc_fn free_alloc) {
    if (!errors) {
        return;
    }
    assert(free_alloc);

    MutSlice error;
    for (size_t i = 0; i < errors->length; i++) {
        UNREACHABLE_IF_ERROR(array_list_get(errors, i, &error));
        free_alloc(error.ptr);
    }

    array_list_clear_retaining_capacity(errors);
}

TRY_STATUS parser_init(Parser* p, Lexer* l, FileIO* io, Allocator allocator) {
    if (!l) {
        return NULL_PARAMETER;
    }

    PROPAGATE_IF_ERROR(parser_null_init(p, io, allocator));
    PROPAGATE_IF_ERROR_DO(parser_reset(p, l), parser_deinit(p));
    return SUCCESS;
}

TRY_STATUS parser_null_init(Parser* p, FileIO* io, Allocator allocator) {
    if (!p || !io) {
        return NULL_PARAMETER;
    }
    ASSERT_ALLOCATOR(allocator);

    HashSet prefix_functions;
    PROPAGATE_IF_ERROR(_init_prefix(&prefix_functions, allocator));

    HashSet infix_functions;
    PROPAGATE_IF_ERROR_DO(_init_infix(&infix_functions, allocator),
                          hash_set_deinit(&prefix_functions));

    HashSet primitives;
    PROPAGATE_IF_ERROR_DO(_init_primitives(&primitives, allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
    });

    HashMap precedences;
    PROPAGATE_IF_ERROR_DO(_init_precedences(&precedences, allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
        hash_set_deinit(&primitives);
    });

    ArrayList errors;
    PROPAGATE_IF_ERROR_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
        hash_set_deinit(&primitives);
        hash_map_deinit(&precedences);
    });

    *p = (Parser){
        .lexer            = NULL,
        .lexer_index      = 0,
        .current_token    = token_init(END, "", 0, 0, 0),
        .peek_token       = token_init(END, "", 0, 0, 0),
        .prefix_parse_fns = prefix_functions,
        .infix_parse_fns  = infix_functions,
        .primitives       = primitives,
        .precedences      = precedences,
        .errors           = errors,
        .io               = io,
        .allocator        = allocator,
    };

    return SUCCESS;
}

TRY_STATUS parser_reset(Parser* p, Lexer* l) {
    if (!l) {
        return NULL_PARAMETER;
    }

    p->lexer         = l;
    p->lexer_index   = 0;
    p->current_token = token_init(END, "", 0, 0, 0);
    p->peek_token    = token_init(END, "", 0, 0, 0);

    clear_error_list(&p->errors, p->allocator.free_alloc);

    // Read twice to set current and peek
    PROPAGATE_IF_ERROR_DO(parser_next_token(p), parser_deinit(p));
    PROPAGATE_IF_ERROR_DO(parser_next_token(p), parser_deinit(p));
    return SUCCESS;
}

void parser_deinit(Parser* p) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    clear_error_list(&p->errors, p->allocator.free_alloc);
    array_list_deinit(&p->errors);
    hash_set_deinit(&p->prefix_parse_fns);
    hash_set_deinit(&p->infix_parse_fns);
    hash_set_deinit(&p->primitives);
    hash_map_deinit(&p->precedences);
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

    clear_statement_list(&ast->statements, ast->allocator.free_alloc);
    clear_error_list(&p->errors, p->allocator.free_alloc);

    // Traverse the tokens and append until exhausted
    while (!parser_current_token_is(p, END)) {
        if (!parser_current_token_is(p, COMMENT)) {
            Statement* stmt = NULL;
            PROPAGATE_IF_ERROR_IS(parser_parse_statement(p, &stmt), ALLOCATION_FAILED);
            if (stmt) {
                PROPAGATE_IF_ERROR_DO(array_list_push(&ast->statements, &stmt),
                                      NODE_VIRTUAL_FREE(stmt, p->allocator.free_alloc));
            }
        }

        IGNORE_STATUS(parser_next_token(p));
    }

    // If we encountered any errors, invalidate the tree for now
    if (p->errors.length > 0) {
        clear_statement_list(&ast->statements, ast->allocator.free_alloc);
    }

    return SUCCESS;
}

TRY_STATUS parser_next_token(Parser* p) {
    assert(p);
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

TRY_STATUS parser_expect_current(Parser* p, TokenType t) {
    assert(p);
    if (parser_current_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        PROPAGATE_IF_ERROR_IS(parser_current_error(p, t), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }
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

static inline TRY_STATUS _parser_error(Parser* p, TokenType t, Token actual_tok) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    StringBuilder builder;
    PROPAGATE_IF_ERROR(string_builder_init_allocator(&builder, 60, p->allocator));

    // Genreal token information
    const char start[] = "Expected token ";
    const char mid[]   = ", found ";

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, start, sizeof(start) - 1),
                          string_builder_deinit(&builder));

    const char* expected = token_type_name(t);
    PROPAGATE_IF_ERROR_DO(string_builder_append_str_z(&builder, expected),
                          string_builder_deinit(&builder));

    PROPAGATE_IF_ERROR_DO(string_builder_append_many(&builder, mid, sizeof(mid) - 1),
                          string_builder_deinit(&builder));

    const char* actual_name = token_type_name(actual_tok.type);
    PROPAGATE_IF_ERROR_DO(string_builder_append_str_z(&builder, actual_name),
                          string_builder_deinit(&builder));

    // Append line/col information for debugging
    PROPAGATE_IF_ERROR_DO(error_append_ln_col(actual_tok.line, actual_tok.column, &builder),
                          string_builder_deinit(&builder));

    MutSlice slice;
    PROPAGATE_IF_ERROR_DO(string_builder_to_string(&builder, &slice),
                          string_builder_deinit(&builder));
    PROPAGATE_IF_ERROR_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&builder));
    return SUCCESS;
}

TRY_STATUS parser_current_error(Parser* p, TokenType t) {
    return _parser_error(p, t, p->current_token);
}

TRY_STATUS parser_peek_error(Parser* p, TokenType t) {
    return _parser_error(p, t, p->peek_token);
}

Precedence parser_current_precedence(Parser* p) {
    assert(p);
    Precedence current;
    if (STATUS_OK(hash_map_get_value(&p->precedences, &p->current_token.type, &current))) {
        return current;
    }
    return LOWEST;
}

Precedence parser_peek_precedence(Parser* p) {
    assert(p);
    Precedence peek;
    if (STATUS_OK(hash_map_get_value(&p->precedences, &p->peek_token.type, &peek))) {
        return peek;
    }
    return LOWEST;
}

TRY_STATUS parser_parse_statement(Parser* p, Statement** stmt) {
    assert(p);
    switch (p->current_token.type) {
    case VAR:
    case CONST:
        PROPAGATE_IF_ERROR(decl_statement_parse(p, (DeclStatement**)stmt));
        break;
    case TYPE:
        PROPAGATE_IF_ERROR(type_decl_statement_parse(p, (TypeDeclStatement**)stmt));
        break;
    case BREAK:
    case RETURN:
        PROPAGATE_IF_ERROR(jump_statement_parse(p, (JumpStatement**)stmt));
        break;
    default:
        PROPAGATE_IF_ERROR(expression_statement_parse(p, (ExpressionStatement**)stmt));
        break;
    }

    return SUCCESS;
}

TRY_STATUS parser_put_status_error(Parser* p, Status status, size_t line, size_t col) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    assert(STATUS_ERR(status));

    StringBuilder builder;
    PROPAGATE_IF_ERROR(string_builder_init_allocator(&builder, 30, p->allocator));

    const char* status_literal = status_name(status);
    PROPAGATE_IF_ERROR_DO(string_builder_append_str_z(&builder, status_literal),
                          string_builder_deinit(&builder));

    PROPAGATE_IF_ERROR_DO(error_append_ln_col(line, col, &builder),
                          string_builder_deinit(&builder));

    MutSlice slice;
    PROPAGATE_IF_ERROR_DO(string_builder_to_string(&builder, &slice),
                          string_builder_deinit(&builder));
    PROPAGATE_IF_ERROR_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&builder));
    return SUCCESS;
}

TRY_STATUS error_append_ln_col(size_t line, size_t col, StringBuilder* sb) {
    assert(sb);
    const char line_no[] = " [Ln ";
    const char col_no[]  = ", Col ";

    PROPAGATE_IF_ERROR(string_builder_append_many(sb, line_no, sizeof(line_no) - 1));
    PROPAGATE_IF_ERROR(string_builder_append_unsigned(sb, (uint64_t)line));
    PROPAGATE_IF_ERROR(string_builder_append_many(sb, col_no, sizeof(col_no) - 1));
    PROPAGATE_IF_ERROR(string_builder_append_unsigned(sb, (uint64_t)col));
    PROPAGATE_IF_ERROR(string_builder_append(sb, ']'));

    return SUCCESS;
}
