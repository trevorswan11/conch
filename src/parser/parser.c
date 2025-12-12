#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "lexer/token.h"
#include "parser/expression_parsers.h"
#include "parser/parser.h"
#include "parser/statement_parsers.h"

#include "ast/ast.h"
#include "ast/expressions/type.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/discard.h"
#include "ast/statements/expression.h"
#include "ast/statements/impl.h"
#include "ast/statements/import.h"
#include "ast/statements/jump.h"

#include "util/containers/string_builder.h"
#include "util/status.h"

static inline NODISCARD Status _init_prefix(HashSet* prefix_set, Allocator allocator) {
    const size_t num_prefix = sizeof(PREFIX_FUNCTIONS) / sizeof(PREFIX_FUNCTIONS[0]);
    TRY(hash_set_init_allocator(prefix_set,
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

static inline NODISCARD Status _init_infix(HashSet* infix_set, Allocator allocator) {
    const size_t num_infix = sizeof(INFIX_FUNCTIONS) / sizeof(INFIX_FUNCTIONS[0]);
    const size_t num_pairs = sizeof(PRECEDENCE_PAIRS) / sizeof(PRECEDENCE_PAIRS[0]);
    if (num_infix < num_pairs) {
        return VIOLATED_INVARIANT;
    }

    TRY(hash_set_init_allocator(infix_set,
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

static inline NODISCARD Status _init_primitives(HashSet* primitive_set, Allocator allocator) {
    const size_t num_primitives = sizeof(ALL_PRIMITIVES) / sizeof(ALL_PRIMITIVES[0]);
    TRY(hash_set_init_allocator(primitive_set,
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

static inline NODISCARD Status _init_precedences(HashMap* precedence_map, Allocator allocator) {
    const size_t num_pairs = sizeof(PRECEDENCE_PAIRS) / sizeof(PRECEDENCE_PAIRS[0]);
    TRY(hash_map_init_allocator(precedence_map,
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

NODISCARD Status parser_init(Parser* p, Lexer* l, FileIO* io, Allocator allocator) {
    assert(p && l && io);
    ASSERT_ALLOCATOR(allocator);

    TRY(parser_null_init(p, io, allocator));
    TRY_DO(parser_reset(p, l), parser_deinit(p));
    return SUCCESS;
}

NODISCARD Status parser_null_init(Parser* p, FileIO* io, Allocator allocator) {
    assert(p && io);
    ASSERT_ALLOCATOR(allocator);

    HashSet prefix_functions;
    TRY(_init_prefix(&prefix_functions, allocator));

    HashSet infix_functions;
    TRY_DO(_init_infix(&infix_functions, allocator), hash_set_deinit(&prefix_functions));

    HashSet primitives;
    TRY_DO(_init_primitives(&primitives, allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
    });

    HashMap precedences;
    TRY_DO(_init_precedences(&precedences, allocator), {
        hash_set_deinit(&prefix_functions);
        hash_set_deinit(&infix_functions);
        hash_set_deinit(&primitives);
    });

    ArrayList errors;
    TRY_DO(array_list_init_allocator(&errors, 10, sizeof(MutSlice), allocator), {
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

NODISCARD Status parser_reset(Parser* p, Lexer* l) {
    assert(l);

    p->lexer         = l;
    p->lexer_index   = 0;
    p->current_token = token_init(END, "", 0, 0, 0);
    p->peek_token    = token_init(END, "", 0, 0, 0);

    clear_error_list(&p->errors, p->allocator.free_alloc);

    // Read twice to set current and peek
    TRY_DO(parser_next_token(p), parser_deinit(p));
    TRY_DO(parser_next_token(p), parser_deinit(p));
    return SUCCESS;
}

void parser_deinit(Parser* p) {
    if (!p) {
        return;
    }
    ASSERT_ALLOCATOR(p->allocator);

    free_error_list(&p->errors, p->allocator.free_alloc);
    hash_set_deinit(&p->prefix_parse_fns);
    hash_set_deinit(&p->infix_parse_fns);
    hash_set_deinit(&p->primitives);
    hash_map_deinit(&p->precedences);
}

NODISCARD Status parser_consume(Parser* p, AST* ast) {
    assert(p && p->lexer && p->io);
    assert(ast);
    ASSERT_ALLOCATOR(ast->allocator);

    p->lexer_index   = 0;
    p->current_token = token_init(END, "", 0, 0, 0);
    p->peek_token    = token_init(END, "", 0, 0, 0);

    TRY(parser_next_token(p));
    TRY(parser_next_token(p));

    clear_statement_list(&ast->statements, ast->allocator.free_alloc);
    clear_error_list(&p->errors, p->allocator.free_alloc);

    // Traverse the tokens and append until exhausted
    while (!parser_current_token_is(p, END)) {
        if (!parser_current_token_is(p, COMMENT)) {
            Statement* stmt = NULL;
            TRY_IS(parser_parse_statement(p, &stmt), ALLOCATION_FAILED);
            if (stmt) {
                TRY_DO(array_list_push(&ast->statements, &stmt),
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

NODISCARD Status parser_next_token(Parser* p) {
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

NODISCARD Status parser_expect_current(Parser* p, TokenType t) {
    assert(p);
    if (parser_current_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        TRY_IS(parser_current_error(p, t), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }
}

NODISCARD Status parser_expect_peek(Parser* p, TokenType t) {
    assert(p);
    if (parser_peek_token_is(p, t)) {
        return parser_next_token(p);
    } else {
        TRY_IS(parser_peek_error(p, t), REALLOCATION_FAILED);
        return UNEXPECTED_TOKEN;
    }
}

static inline NODISCARD Status _parser_error(Parser* p, TokenType t, Token actual_tok) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);

    StringBuilder builder;
    TRY(string_builder_init_allocator(&builder, 60, p->allocator));

    // Genreal token information
    const char start[] = "Expected token ";
    const char mid[]   = ", found ";

    TRY_DO(string_builder_append_many(&builder, start, sizeof(start) - 1),
           string_builder_deinit(&builder));

    const char* expected = token_type_name(t);
    TRY_DO(string_builder_append_str_z(&builder, expected), string_builder_deinit(&builder));

    TRY_DO(string_builder_append_many(&builder, mid, sizeof(mid) - 1),
           string_builder_deinit(&builder));

    const char* actual_name = token_type_name(actual_tok.type);
    TRY_DO(string_builder_append_str_z(&builder, actual_name), string_builder_deinit(&builder));

    // Append line/col information for debugging
    TRY_DO(error_append_ln_col(actual_tok.line, actual_tok.column, &builder),
           string_builder_deinit(&builder));

    MutSlice slice;
    TRY_DO(string_builder_to_string(&builder, &slice), string_builder_deinit(&builder));
    TRY_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&builder));
    return SUCCESS;
}

NODISCARD Status parser_current_error(Parser* p, TokenType t) {
    return _parser_error(p, t, p->current_token);
}

NODISCARD Status parser_peek_error(Parser* p, TokenType t) {
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

NODISCARD Status parser_parse_statement(Parser* p, Statement** stmt) {
    assert(p);
    switch (p->current_token.type) {
    case VAR:
    case CONST:
        TRY(decl_statement_parse(p, (DeclStatement**)stmt));
        break;
    case TYPE:
        TRY(type_decl_statement_parse(p, (TypeDeclStatement**)stmt));
        break;
    case BREAK:
    case RETURN:
    case CONTINUE:
        TRY(jump_statement_parse(p, (JumpStatement**)stmt));
        break;
    case IMPL:
        TRY(impl_statement_parse(p, (ImplStatement**)stmt));
        break;
    case IMPORT:
        TRY(import_statement_parse(p, (ImportStatement**)stmt));
        break;
    case LBRACE:
        TRY(block_statement_parse(p, (BlockStatement**)stmt));
        break;
    case UNDERSCORE:
        TRY(discard_statement_parse(p, (DiscardStatement**)stmt));
        break;
    default:
        TRY(expression_statement_parse(p, (ExpressionStatement**)stmt));
        break;
    }

    return SUCCESS;
}

NODISCARD Status parser_put_status_error(Parser* p, Status status, size_t line, size_t col) {
    assert(p);
    ASSERT_ALLOCATOR(p->allocator);
    assert(STATUS_ERR(status));

    StringBuilder builder;
    TRY(string_builder_init_allocator(&builder, 30, p->allocator));

    const char* status_literal = status_name(status);
    TRY_DO(string_builder_append_str_z(&builder, status_literal), string_builder_deinit(&builder));

    TRY_DO(error_append_ln_col(line, col, &builder), string_builder_deinit(&builder));

    MutSlice slice;
    TRY_DO(string_builder_to_string(&builder, &slice), string_builder_deinit(&builder));
    TRY_DO(array_list_push(&p->errors, &slice), string_builder_deinit(&builder));
    return SUCCESS;
}

NODISCARD Status error_append_ln_col(size_t line, size_t col, StringBuilder* sb) {
    assert(sb);
    const char line_no[] = " [Ln ";
    const char col_no[]  = ", Col ";

    TRY(string_builder_append_many(sb, line_no, sizeof(line_no) - 1));
    TRY(string_builder_append_unsigned(sb, (uint64_t)line));
    TRY(string_builder_append_many(sb, col_no, sizeof(col_no) - 1));
    TRY(string_builder_append_unsigned(sb, (uint64_t)col));
    TRY(string_builder_append(sb, ']'));

    return SUCCESS;
}
