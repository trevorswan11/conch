#pragma once

#include <span>

#include "util/common.hpp"
#include "util/expected.hpp"
#include "util/optional.hpp"

#include "ast/node.hpp"

#include "parser/parser.hpp"

namespace conch::ast {

class IdentifierExpression;
class TypeExpression;
class FunctionExpression;

class DeclStatement;

class StructMember {
  public:
    explicit StructMember(bool                      priv,
                          Box<IdentifierExpression> name,
                          Box<TypeExpression>       type,
                          Optional<Box<Expression>> default_value) noexcept;
    ~StructMember();

    StructMember(const StructMember&)                        = delete;
    auto operator=(const StructMember&) -> StructMember&     = delete;
    StructMember(StructMember&&) noexcept                    = default;
    auto operator=(StructMember&&) noexcept -> StructMember& = default;

    [[nodiscard]] auto is_private() const noexcept -> bool { return private_; }
    [[nodiscard]] auto get_name() const noexcept -> const IdentifierExpression& { return *name_; }
    [[nodiscard]] auto get_type() const noexcept -> const TypeExpression& { return *type_; }
    [[nodiscard]] auto has_default_value() const noexcept -> bool {
        return default_value_.has_value();
    }

    [[nodiscard]] auto get_default_value() const noexcept -> Optional<const Expression&> {
        return default_value_ ? Optional<const Expression&>{**default_value_} : nullopt;
    }

    friend class StructExpression;

  private:
    bool                      private_;
    Box<IdentifierExpression> name_;
    Box<TypeExpression>       type_;
    Optional<Box<Expression>> default_value_;
};

class StructExpression : public KindExpression<StructExpression> {
  public:
    static constexpr auto KIND = NodeKind::STRUCT_EXPRESSION;

  public:
    explicit StructExpression(const Token&                         start_token,
                              bool                                 packed,
                              std::vector<Box<DeclStatement>>      declarations,
                              std::vector<StructMember>            members,
                              std::vector<Box<FunctionExpression>> functions) noexcept;
    ~StructExpression() override;

    auto accept(Visitor& v) const -> void override;

    [[nodiscard]] static auto parse(Parser& parser) -> Expected<Box<Expression>, ParserDiagnostic>;

    [[nodiscard]] auto is_packed() const noexcept -> bool { return packed_; }
    [[nodiscard]] auto get_decls() const noexcept -> std::span<const Box<DeclStatement>> {
        return declarations_;
    }

    [[nodiscard]] auto get_members() const noexcept -> std::span<const StructMember> {
        return members_;
    }

    [[nodiscard]] auto get_functions() const noexcept -> std::span<const Box<FunctionExpression>> {
        return functions_;
    }

    auto is_equal(const Node& other) const noexcept -> bool override;

  private:
    bool                                 packed_;
    std::vector<Box<DeclStatement>>      declarations_;
    std::vector<StructMember>            members_;
    std::vector<Box<FunctionExpression>> functions_;
};

} // namespace conch::ast
