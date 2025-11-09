#pragma once

#include "ast/ast.h"
#include "ast/statements/declarations.h"
#include "ast/statements/statement.h"

#include "parser/parser.h"

VarStatement* var_statement_parse(Parser* p);
