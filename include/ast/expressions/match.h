#ifndef MATCH_EXPR_H
#define MATCH_EXPR_H

#include "ast/expressions/expression.h"
#include "ast/statements/statement.h"

#include "util/containers/array_list.h"

typedef struct MatchArm {
    Expression* pattern;
    Statement*  dispatch;
} MatchArm;

typedef struct MatchExpression {
    Expression  base;
    Expression* expression;
    ArrayList   arms;
    Statement*  catch_all;
} MatchExpression;

void free_match_arm_list(ArrayList* arms, Allocator* allocator);

[[nodiscard]] Status match_expression_create(Token             start_token,
                                             Expression*       expression,
                                             ArrayList         arms,
                                             Statement*        catch_all,
                                             MatchExpression** match_expr,
                                             Allocator* allocator);

void match_expression_destroy(Node* node, Allocator* allocator);
[[nodiscard]] Status
match_expression_reconstruct(Node* node, const HashMap* symbol_map, StringBuilder* sb);
[[nodiscard]] Status
match_expression_analyze(Node* node, SemanticContext* parent, ArrayList* errors);

static const ExpressionVTable MATCH_VTABLE = {
    .base =
        {
            .destroy     = match_expression_destroy,
            .reconstruct = match_expression_reconstruct,
            .analyze     = match_expression_analyze,
        },
};

#endif
