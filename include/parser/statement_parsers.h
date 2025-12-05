#pragma once

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

TRY_STATUS decl_statement_parse(Parser* p, DeclStatement** stmt);
TRY_STATUS type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt);
TRY_STATUS jump_statement_parse(Parser* p, JumpStatement** stmt);
TRY_STATUS expression_statement_parse(Parser* p, ExpressionStatement** stmt);
TRY_STATUS block_statement_parse(Parser* p, BlockStatement** stmt);
TRY_STATUS impl_statement_parse(Parser* p, ImplStatement** stmt);
TRY_STATUS import_statement_parse(Parser* p, ImportStatement** stmt);
TRY_STATUS discard_statement_parse(Parser* p, DiscardStatement** stmt);
