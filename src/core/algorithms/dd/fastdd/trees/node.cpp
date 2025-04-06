#include "core/algorithms/dd/fastdd/trees/node.h"

namespace algos::dd {

std::unique_ptr<Node> Node::CreateInnerNode(std::unique_ptr<Node> first_leaf,
                                            std::unique_ptr<Node> second_leaf, std::size_t bit) {
    assert(first_leaf->node_type_ == NodeType::LeafNode &&
           second_leaf->node_type_ == NodeType::LeafNode);
    bool first_bit = first_leaf->bitset_.value()[bit];
    bool second_bit = second_leaf->bitset_.value()[bit];
    while (first_bit == second_bit) {
        ++bit;
        first_bit = first_leaf->bitset_.value()[bit];
        second_bit = second_leaf->bitset_.value()[bit];
    }

    boost::dynamic_bitset<> union_bitset =
            boost::operator|(first_leaf->bitset_.value(), second_leaf->bitset_.value());
    boost::dynamic_bitset<> intersect_bitset =
            boost::operator|(first_leaf->bitset_.value(), second_leaf->bitset_.value());

    return std::make_unique<Node>(bit, union_bitset, intersect_bitset,
                                  first_bit ? std::move(second_leaf) : std::move(first_leaf),
                                  first_bit ? std::move(first_leaf) : std::move(second_leaf));
}

std::unique_ptr<Node> Node::Add(std::unique_ptr<Node> this_node,
                                boost::dynamic_bitset<> const& bitset, std::size_t bit) {
    if (this_node->node_type_ == NodeType::EmptyNode) {
        return std::make_unique<Node>(bitset);
    }
    if (this_node->node_type_ == NodeType::LeafNode) {
        if (bitset == this_node->bitset_) {
            return this_node;
        }
        return CreateInnerNode(std::move(this_node), std::make_unique<Node>(bitset), bit);
    }

    while (bit < this_node->bit_) {
        bool bitset_value = bitset[bit];
        bool union_value = this_node->union_.value()[bit];
        if (bitset_value != union_value) {
            boost::dynamic_bitset<> union_bitset =
                    boost::operator|(this_node->union_.value(), bitset);
            boost::dynamic_bitset<> intersect_bitset =
                    boost::operator&(this_node->intersect_.value(), bitset);
            std::unique_ptr<Node> left_node =
                    bitset_value ? std::move(this_node) : std::make_unique<Node>(bitset);
            std::unique_ptr<Node> right_node =
                    bitset_value ? std::make_unique<Node>(bitset) : std::move(this_node);
            return std::make_unique<Node>(bit, union_bitset, intersect_bitset, std::move(left_node),
                                          std::move(right_node));
        }
        ++bit;
    }
    assert(bit == this_node->bit_);

    if (bitset[bit]) {
        this_node->right_child_ =
                this_node->right_child_->Add(std::move(this_node->right_child_), bitset, bit + 1);
    } else {
        this_node->left_child_ =
                this_node->left_child_->Add(std::move(this_node->left_child_), bitset, bit + 1);
    }
    this_node->union_->operator|=(bitset);
    this_node->intersect_->operator&=(bitset);

    return this_node;
}

}  // namespace algos::dd
