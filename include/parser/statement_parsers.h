#ifndef STATEMENT_PARSERS_H
#define STATEMENT_PARSERS_H

#include "util/status.h"

typedef struct Parser              Parser;
typedef struct DeclStatement       DeclStatement;
typedef struct TypeDeclStatement   TypeDeclStatement;
typedef struct JumpStatement       JumpStatement;
typedef struct ExpressionStatement ExpressionStatement;
typedef struct BlockStatement      BlockStatement;
typedef struct ImplStatement       ImplStatement;
typedef struct ImportStatement     ImportStatement;
typedef struct DiscardStatement    DiscardStatement;

NODISCARD Status decl_statement_parse(Parser* p, DeclStatement** stmt);
NODISCARD Status type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt);
NODISCARD Status jump_statement_parse(Parser* p, JumpStatement** stmt);
NODISCARD Status expression_statement_parse(Parser* p, ExpressionStatement** stmt);
NODISCARD Status block_statement_parse(Parser* p, BlockStatement** stmt);
NODISCARD Status impl_statement_parse(Parser* p, ImplStatement** stmt);
NODISCARD Status import_statement_parse(Parser* p, ImportStatement** stmt);
NODISCARD Status discard_statement_parse(Parser* p, DiscardStatement** stmt);

#endif
