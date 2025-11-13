#pragma once

#include "parser/parser.h"

#include "ast/expressions/expression.h"

#include "util/status.h"

typedef TRY_STATUS (*prefix_parse_fn)(Parser*, Expression**);
typedef TRY_STATUS (*infix_parse_fn)(Parser*, Expression*, Expression**);

TRY_STATUS identifier_expression_parse(Parser* p, Expression** expression);
