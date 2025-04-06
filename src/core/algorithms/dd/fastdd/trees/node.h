#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

#include <boost/dynamic_bitset.hpp>

namespace algos::dd {

class Node {
private:
    std::optional<boost::dynamic_bitset<>> bitset_;

    std::size_t bit_ = 0;

    std::optional<boost::dynamic_bitset<>> union_;
    std::optional<boost::dynamic_bitset<>> intersect_;

    std::unique_ptr<Node> left_child_;
    std::unique_ptr<Node> right_child_;

    static std::unique_ptr<Node> CreateInnerNode(std::unique_ptr<Node> first_leaf,
                                                 std::unique_ptr<Node> second_leaf,
                                                 std::size_t bit);

public:
    enum class NodeType { EmptyNode, LeafNode, InnerNode };
    NodeType node_type_ = NodeType::EmptyNode;

    Node() = default;

    Node(boost::dynamic_bitset<> const& bitset, NodeType node_type = NodeType::LeafNode)
        : bitset_(bitset), node_type_(node_type) {}

    Node(std::size_t bit, boost::dynamic_bitset<> const& union_bitset,
         boost::dynamic_bitset<> const& intersect_bitset, std::unique_ptr<Node> left_child,
         std::unique_ptr<Node> right_child, NodeType node_type = NodeType::InnerNode)
        : bit_(bit),
          union_(union_bitset),
          intersect_(intersect_bitset),
          left_child_(std::move(left_child)),
          right_child_(std::move(right_child)),
          node_type_(node_type) {}

    static std::unique_ptr<Node> Add(std::unique_ptr<Node> this_node,
                                     boost::dynamic_bitset<> const& bitset, std::size_t bit);

    boost::dynamic_bitset<> Union() const {
        if (node_type_ == NodeType::EmptyNode) {
            return boost::dynamic_bitset<>();
        }
        if (node_type_ == NodeType::LeafNode) {
            return *bitset_;
        }

        return *union_;
    }

    boost::dynamic_bitset<> Intersect() const {
        if (node_type_ == NodeType::EmptyNode) {
            return boost::dynamic_bitset<>();
        }
        if (node_type_ == NodeType::LeafNode) {
            return *bitset_;
        }

        return *intersect_;
    }

    std::optional<boost::dynamic_bitset<>> FindSuperSet(
            boost::dynamic_bitset<> const& bitset) const {
        if (node_type_ == NodeType::EmptyNode) {
            return std::nullopt;
        }
        if (node_type_ == NodeType::LeafNode) {
            return bitset.is_subset_of(bitset_.value()) ? bitset_ : std::nullopt;
        }
        if (bitset.is_subset_of(union_.value())) {
            std::optional<boost::dynamic_bitset<>> superset = left_child_->FindSuperSet(bitset);
            return superset ? superset : right_child_->FindSuperSet(bitset);
        }

        return std::nullopt;
    }

    boost::dynamic_bitset<> const& GetBitset() const {
        return bitset_.value();
    }

    std::unique_ptr<Node> const& GetLeftChild() const {
        return left_child_;
    }

    std::unique_ptr<Node> const& GetRightChild() const {
        return right_child_;
    }
};

}  // namespace algos::dd
