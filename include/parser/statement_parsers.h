#pragma once

#include "ast/ast.h"
#include "ast/statements/block.h"
#include "ast/statements/declarations.h"
#include "ast/statements/expression.h"
#include "ast/statements/jump.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

#include "util/status.h"

TRY_STATUS decl_statement_parse(Parser* p, DeclStatement** stmt);
TRY_STATUS jump_statement_parse(Parser* p, JumpStatement** stmt);
TRY_STATUS expression_statement_parse(Parser* p, ExpressionStatement** stmt);
TRY_STATUS block_statement_parse(Parser* p, BlockStatement** stmt);
