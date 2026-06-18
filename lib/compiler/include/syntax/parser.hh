#pragma once

#include <string_view>
#include <utility>

#include <stdx/option.hh>
#include <stdx/result.hh>
#include <stdx/types.hh>

#include "ast/ast.hh"
#include "ast/handle.hh"
#include "ast/id.hh"
#include "ast/kind.hh"
#include "syntax/error.hh"
#include "syntax/lexer.hh"
#include "syntax/precedence.hh"
#include "syntax/token.hh"
#include "syntax/token_type.hh"

#include <diagnostic.hh>

namespace ghoti::ast { class AST; } // namespace ghoti::ast

namespace ghoti::syntax {

class Parser {
  public:
    using PrefixFn = stdx::Result<ast::ExpressionHandle, Diagnostic> (*)(Parser&);
    using InfixFn  = stdx::Result<ast::ExpressionHandle, Diagnostic> (*)(Parser&,
                                                                        ast::ExpressionHandle);

    class Checkpoint {
      public:
        explicit Checkpoint(const Parser& parser) noexcept
            : snapshot_{parser.lexer_}, current_{parser.current_token_}, peek_{parser.peek_token_} {
        }

      private:
        Lexer::Snapshot snapshot_;
        Token           current_;
        Token           peek_;

        friend class Parser;
    };

    // An RAII checkpoint-rollback transaction
    class Transaction {
      public:
        explicit Transaction(Parser& parser) noexcept : parser_{parser}, checkpoint_{parser} {}
        ~Transaction() {
            if (!committed_) { parser_.rollback(checkpoint_); }
        }

        // Prevent a rollback from happening at the end of the transaction
        void commit() noexcept { committed_ = true; }

      private:
        Parser&            parser_;
        Parser::Checkpoint checkpoint_;
        bool               committed_{false};
    };

  public:
    Parser() noexcept = default;
    explicit Parser(std::string_view input) noexcept { reset(input); }

    auto reset(std::string_view input = {}) noexcept -> void;

    // Advances the parser, returning the resulting current token.
    // This is a no-op at end of stream.
    auto advance(u8 times = 1) noexcept -> const Token&;

    // Fills the AST with the parser's output, clearing it before use
    auto consume(ast::AST& ast) -> Diagnostics;

    [[nodiscard]] auto get_current_token() const noexcept -> const Token& { return current_token_; }
    [[nodiscard]] auto get_peek_token() const noexcept -> const Token& { return peek_token_; }

    [[nodiscard]] auto current_token_is(TokenType t) const noexcept -> bool {
        return current_token_.type == t;
    }
    [[nodiscard]] auto peek_token_is(TokenType t) const noexcept -> bool {
        return peek_token_.type == t;
    }

    // Advances the cursor tokens only if the expected token type matches the actual peek token.
    [[nodiscard]] auto expect_peek(TokenType expected) -> stdx::Result<void, Diagnostic>;

    // Checks for a semicolon in either the current or peak token and advances state accordingly
    //
    // Only use this over `expect_peek` when a potentially-block expr has just been parsed
    [[nodiscard]] auto expect_semicolon() -> stdx::Result<void, Diagnostic>;

    // Indiscriminately returns an error citing the peek token.
    [[nodiscard]] auto peek_error(TokenType expected) -> Diagnostic;

    [[nodiscard]] auto get_current_precedence() const noexcept
        -> std::pair<Precedence, stdx::Option<Binding>>;
    [[nodiscard]] auto get_peek_precedence() const noexcept
        -> std::pair<Precedence, stdx::Option<Binding>>;

    [[nodiscard]] auto parse_statement(SemicolonBehavior behavior = SemicolonBehavior::REQUIRE)
        -> stdx::Result<ast::StatementHandle, Diagnostic>;
    [[nodiscard]] auto parse_expression(Precedence precedence = Precedence::LOWEST)
        -> stdx::Result<ast::ExpressionHandle, Diagnostic>;

    // Assumes that the current token is looking at the start of the expression.
    // The resulting statement can only be a jump, block, or expression statement.
    [[nodiscard]] auto
    parse_restricted_statement(Error error, SemicolonBehavior behavior = SemicolonBehavior::REQUIRE)
        -> stdx::Result<ast::StatementHandle, Diagnostic>;

    // Parses a restricted statement only if an else token is currently looked at.
    [[nodiscard]] auto
    try_parse_restricted_alternate(Error             error,
                                   SemicolonBehavior behavior = SemicolonBehavior::REQUIRE)
        -> stdx::Result<stdx::Option<ast::StatementHandle>, Diagnostic>;

    static auto get_prefix_fn_opt(TokenType tt) noexcept -> stdx::Option<PrefixFn>;
    static auto get_poll_infix_fn_opt(TokenType tt) noexcept -> stdx::Option<InfixFn>;

    [[nodiscard]] auto get_ast() noexcept -> ast ::AST& { return *ast_; }

    template <traits::ASTNode N> [[nodiscard]] constexpr auto get_node(ast::NodeID id) -> const N& {
        return ast_->get_as<N>(id);
    }

    [[nodiscard]] auto get_location_of(ast::NodeID id) -> SourceLocation;
    [[nodiscard]] auto get_location_of(ast::ExplicitTypeID id) -> SourceLocation;

    // Adds an expression to the ast and returns its handle
    template <traits::ASTNode Data, typename... Args>
    [[nodiscard]] constexpr auto add_expr(const syntax::Token& start_token, Args&&... args) {
        return add_node<ast::ExpressionHandle, Data>(start_token, std::forward<Args>(args)...);
    }

    // Adds a statement to the ast and returns its handle
    template <traits::ASTNode Data, typename... Args>
    [[nodiscard]] constexpr auto add_stmt(const syntax::Token& start_token, Args&&... args) {
        return add_node<ast::StatementHandle, Data>(start_token, std::forward<Args>(args)...);
    }

    // Adds a node to the ast and casts the result to the requested handle
    template <typename Handle, traits::ASTNode Data, typename... Args>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Args&&... args) {
        return Handle{ast_->add_node(start_token, Data{std::forward<Args>(args)...})};
    }

    // Helper for type-ast insertion, reducing a layer of call-site indirection
    template <traits::ASTExplicitType Data, typename... Args>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, ast::TypeModifier mod, Args&&... args) {
        return ast_->add_type(start_token, mod, Data{std::forward<Args>(args)...});
    }

  private:
    // Reverts the parser to the state from the checkpoint.
    auto rollback(const Checkpoint& checkpoint) noexcept -> void {
        lexer_.restore(checkpoint.snapshot_);
        current_token_ = checkpoint.current_;
        peek_token_    = checkpoint.peek_;
    }

  private:
    std::string_view        input_;
    Lexer                   lexer_;
    Token                   current_token_;
    Token                   peek_token_;
    stdx::Option<ast::AST&> ast_;
};

} // namespace ghoti::syntax
