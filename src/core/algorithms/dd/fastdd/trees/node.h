#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class Node {
private:
    std::optional<Bitset> bitset_;

    std::size_t bit_ = 0;

    std::optional<Bitset> union_;
    std::optional<Bitset> intersect_;

    std::unique_ptr<Node<Bitset>> left_child_;
    std::unique_ptr<Node<Bitset>> right_child_;

    static std::unique_ptr<Node<Bitset>> CreateInnerNode(std::unique_ptr<Node<Bitset>> first_leaf,
                                                         std::unique_ptr<Node<Bitset>> second_leaf,
                                                         std::size_t bit) {
        assert(first_leaf->node_type_ == NodeType::LeafNode &&
               second_leaf->node_type_ == NodeType::LeafNode);
        bool first_bit = first_leaf->bitset_.value()[bit];
        bool second_bit = second_leaf->bitset_.value()[bit];
        while (first_bit == second_bit) {
            ++bit;
            first_bit = first_leaf->bitset_.value()[bit];
            second_bit = second_leaf->bitset_.value()[bit];
        }

        Bitset union_bitset = first_leaf->bitset_.value();
        union_bitset |= second_leaf->bitset_.value();

        Bitset intersect_bitset = first_leaf->bitset_.value();
        intersect_bitset &= second_leaf->bitset_.value();

        return std::make_unique<Node<Bitset>>(
                bit, union_bitset, intersect_bitset,
                first_bit ? std::move(second_leaf) : std::move(first_leaf),
                first_bit ? std::move(first_leaf) : std::move(second_leaf));
    }

public:
    enum class NodeType { EmptyNode, LeafNode, InnerNode };
    NodeType node_type_ = NodeType::EmptyNode;

    Node() = default;

    Node(Bitset const& bitset, NodeType node_type = NodeType::LeafNode)
        : bitset_(bitset), node_type_(node_type) {}

    Node(std::size_t bit, Bitset const& union_bitset, Bitset const& intersect_bitset,
         std::unique_ptr<Node<Bitset>> left_child, std::unique_ptr<Node<Bitset>> right_child,
         NodeType node_type = NodeType::InnerNode)
        : bit_(bit),
          union_(union_bitset),
          intersect_(intersect_bitset),
          left_child_(std::move(left_child)),
          right_child_(std::move(right_child)),
          node_type_(node_type) {}

    static std::unique_ptr<Node<Bitset>> Add(std::unique_ptr<Node<Bitset>> this_node,
                                             Bitset const& bitset, std::size_t bit) {
        if (this_node->node_type_ == NodeType::EmptyNode) {
            return std::make_unique<Node<Bitset>>(bitset);
        }
        if (this_node->node_type_ == NodeType::LeafNode) {
            if (bitset == this_node->bitset_) {
                return this_node;
            }
            return CreateInnerNode(std::move(this_node), std::make_unique<Node<Bitset>>(bitset),
                                   bit);
        }

        while (bit < this_node->bit_) {
            bool bitset_value = bitset[bit];
            bool union_value = this_node->union_.value()[bit];
            if (bitset_value != union_value) {
                Bitset union_bitset = this_node->union_.value();
                union_bitset |= bitset;
                Bitset intersect_bitset = this_node->union_.value();
                intersect_bitset &= bitset;

                std::unique_ptr<Node<Bitset>> left_node =
                        bitset_value ? std::move(this_node)
                                     : std::make_unique<Node<Bitset>>(bitset);
                std::unique_ptr<Node<Bitset>> right_node =
                        bitset_value ? std::make_unique<Node<Bitset>>(bitset)
                                     : std::move(this_node);
                return std::make_unique<Node<Bitset>>(bit, union_bitset, intersect_bitset,
                                                      std::move(left_node), std::move(right_node));
            }
            ++bit;
        }
        assert(bit == this_node->bit_);

        if (bitset[bit]) {
            this_node->right_child_ = this_node->right_child_->Add(
                    std::move(this_node->right_child_), bitset, bit + 1);
        } else {
            this_node->left_child_ =
                    this_node->left_child_->Add(std::move(this_node->left_child_), bitset, bit + 1);
        }
        this_node->union_->operator|=(bitset);
        this_node->intersect_->operator&=(bitset);

        return this_node;
    }

    Bitset Union() const {
        if (node_type_ == NodeType::EmptyNode) {
            return Bitset();
        }
        if (node_type_ == NodeType::LeafNode) {
            return *bitset_;
        }

        return *union_;
    }

    Bitset Intersect() const {
        if (node_type_ == NodeType::EmptyNode) {
            return Bitset();
        }
        if (node_type_ == NodeType::LeafNode) {
            return *bitset_;
        }

        return *intersect_;
    }

    std::optional<Bitset> FindSuperSet(Bitset const& bitset) const {
        if (node_type_ == NodeType::EmptyNode) {
            return std::nullopt;
        }
        if (node_type_ == NodeType::LeafNode) {
            return bitset.is_subset_of(bitset_.value()) ? bitset_ : std::nullopt;
        }
        if (bitset.is_subset_of(union_.value())) {
            std::optional<Bitset> superset = left_child_->FindSuperSet(bitset);
            return superset ? superset : right_child_->FindSuperSet(bitset);
        }

        return std::nullopt;
    }

    Bitset const& GetBitset() const {
        return bitset_.value();
    }

    std::unique_ptr<Node<Bitset>> const& GetLeftChild() const {
        return left_child_;
    }

    std::unique_ptr<Node<Bitset>> const& GetRightChild() const {
        return right_child_;
    }
};

}  // namespace algos::dd
