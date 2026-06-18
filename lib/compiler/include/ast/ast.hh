#pragma once

#include <vector>

#include <stdx/assert.hh>
#include <stdx/iterator.hh>
#include <stdx/option.hh>
#include <stdx/types.hh>
#include <stdx/utility.hh>

#include "ast/id.hh"
#include "ast/kind.hh"
#include "ast/traits.hh"
#include "ast/visitor.hh"
#include "syntax/token.hh"

#include <diagnostic.hh>

namespace ghoti::ast {

template <traits::IndexableID ID, typename Data> struct DataPoolBase {
    std::vector<Data>           pool;
    std::vector<SourceLocation> locations;

    constexpr auto clear() noexcept -> void {
        pool.clear();
        locations.clear();
    }

    constexpr auto emplace_back(const syntax::Token& start_token, Data&& data) -> u64 {
        const u64 index{pool.size()};
        pool.emplace_back(std::forward<Data>(data));
        locations.emplace_back(traits::SourceInfo<syntax::Token>::get(start_token));
        return index;
    }
};

template <typename ID, typename Data> struct DataPool : public DataPoolBase<ID, Data> {};
template <typename Data> struct DataPool<NodeID, Data> : public DataPoolBase<NodeID, Data> {
    std::vector<NodeID> roots;

    constexpr auto clear() noexcept -> void {
        roots.clear();
        DataPoolBase<NodeID, Data>::clear();
    }
};

// A collection of multiple AST tree structures
class AST {
  public:
    struct DataPoolSizes {
        usize nodes_size;
        usize types_size;
    };

  public:
    MAKE_UNALIASED_ITERATOR(std::vector<NodeID>, nodes_.roots)

  public:
    AST()  = default;
    ~AST() = default;
    MAKE_MOVE_ONLY(AST)

    constexpr auto add_root(NodeID id) -> void { nodes_.roots.push_back(id); }

    [[nodiscard]] constexpr auto get_pool_sizes() const noexcept -> DataPoolSizes {
        return {.nodes_size = nodes_.pool.size(), .types_size = explicit_types_.pool.size()};
    }

    template <traits::ASTNode Data>
    [[nodiscard]] constexpr auto add_node(const syntax::Token& start_token, Data&& data) -> NodeID {
        constexpr auto kind{traits::NodeKindOf<Data>::value()};
        const auto     index{nodes_.emplace_back(start_token, std::forward<Data>(data))};
        return NodeID{kind, start_token.type, index};
    }

    template <traits::ASTExplicitType Data>
    [[nodiscard]] constexpr auto
    add_type(const syntax::Token& start_token, TypeModifier mod, Data&& data) -> ExplicitTypeID {
        constexpr auto kind{traits::ExplicitTypeKindOf<Data>::value()};
        const auto     index{explicit_types_.emplace_back(start_token, std::forward<Data>(data))};
        return ExplicitTypeID{kind, mod, start_token.type, index};
    }

    template <traits::IndexableID ID>
    [[nodiscard]] constexpr auto location_of(ID id) const noexcept -> const SourceLocation& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        if constexpr (traits::IndexableNodeID<ID>) {
            return nodes_.locations[id.get_index()];
        } else {
            return explicit_types_.locations[id.get_index()];
        }
    }

    // Returns the node data at the provided id
    template <traits::IndexableID ID>
    [[nodiscard]] constexpr auto operator[](ID id) const noexcept -> auto& {
        ASSERT(id.is_valid(), "Attempt to access invalid id");
        if constexpr (traits::IndexableNodeID<ID>) {
            return nodes_.pool[id.get_index()];
        } else {
            return explicit_types_.pool[id.get_index()];
        }
    }

    // Returns the casted node at the requested index
    template <typename Data, traits::IndexableID ID>
    [[nodiscard]] constexpr auto get_as(ID id) const noexcept -> const Data& {
        ASSERT(id.template is<Data>(), "Illegal node data retrieval");
        return operator[](id).template as<Data>();
    }

    // Returns the casted node data at the requested index if present
    template <typename Data, traits::IndexableID ID>
    [[nodiscard]] constexpr auto get_as_opt(ID id) const noexcept -> stdx::Option<const Data&> {
        return operator[](id).template as_opt<Data>();
    }

    constexpr auto clear() noexcept -> void {
        nodes_.clear();
        explicit_types_.clear();
    }

  private:
    DataPool<NodeID, NodeData>         nodes_;
    DataPool<ExplicitTypeID, TypeData> explicit_types_;
};

} // namespace ghoti::ast
