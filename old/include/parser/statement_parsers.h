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

[[nodiscard]] Status decl_statement_parse(Parser* p, DeclStatement** stmt);
[[nodiscard]] Status type_decl_statement_parse(Parser* p, TypeDeclStatement** stmt);
[[nodiscard]] Status jump_statement_parse(Parser* p, JumpStatement** stmt);
[[nodiscard]] Status expression_statement_parse(Parser* p, ExpressionStatement** stmt);
[[nodiscard]] Status block_statement_parse(Parser* p, BlockStatement** stmt);
[[nodiscard]] Status impl_statement_parse(Parser* p, ImplStatement** stmt);
[[nodiscard]] Status import_statement_parse(Parser* p, ImportStatement** stmt);
[[nodiscard]] Status discard_statement_parse(Parser* p, DiscardStatement** stmt);

#endif
