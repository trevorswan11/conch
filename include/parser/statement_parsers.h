#pragma once

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

#include "util/status.h"

TRY_STATUS decl_statement_parse(Parser* p, DeclStatement** stmt);
TRY_STATUS return_statement_parse(Parser* p, ReturnStatement** stmt);
