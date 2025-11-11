#pragma once

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

#include "util/error.h"

AnyError decl_statement_parse(Parser* p, bool constant, DeclStatement** stmt);
AnyError return_statement_parse(Parser* p, ReturnStatement** stmt);
