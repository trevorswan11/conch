#pragma once

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/return.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

DeclStatement*   decl_statement_parse(Parser* p, bool constant);
ReturnStatement* return_statement_parse(Parser* p);
