#pragma once

#include <memory>
#include <optional>
#include <stack>
#include <utility>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/dd/fastdd/trees/node.h"
#include "core/algorithms/dd/fastdd/util/bitset_concept.h"

namespace algos::dd {

template <BoostDynamicBitsetCompatible Bitset>
class TreeSearch {
private:
    std::unique_ptr<Node<Bitset>> root_ = std::make_unique<Node<Bitset>>();

public:
    void Add(Bitset const& bitset) {
        std::unique_ptr<Node<Bitset>> new_root = Node<Bitset>::Add(std::move(root_), bitset, 0);
        root_ = std::move(new_root);
    }

    std::optional<Bitset> FindSuperSet(Bitset const& bitset) const {
        return root_->FindSuperSet(bitset);
    }

    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Bitset;
        using pointer = value_type const*;
        using reference = value_type const&;

        Iterator(Node<Bitset>* root, bool is_end = false) : traversal_() {
            if (!is_end && root->node_type_ != Node<Bitset>::NodeType::EmptyNode) {
                traversal_.emplace(root);
                FindNext();
            }
        }

        reference operator*() const {
            Node<Bitset>* cur_node = traversal_.top().node;
            return cur_node->GetBitset();
        }

        Iterator& operator++() {
            StackNode cur_node = traversal_.top();
            while (cur_node.node->node_type_ == Node<Bitset>::NodeType::LeafNode ||
                   cur_node.is_right_child) {
                traversal_.pop();
                if (traversal_.empty()) {
                    break;
                }
                cur_node = traversal_.top();
            }
            if (!traversal_.empty()) {
                traversal_.top().is_right_child = true;
                FindNext();
            }

            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.traversal_ == b.traversal_;
        }

        friend bool operator!=(Iterator const& a, Iterator const& b) {
            return !(a == b);
        }

    private:
        struct StackNode {
            Node<Bitset>* node;
            bool is_right_child;

            bool operator==(StackNode const& other) const = default;
        };

        void FindNext() {
            while (!traversal_.empty()) {
                StackNode cur_node = traversal_.top();
                if (cur_node.node->node_type_ == Node<Bitset>::NodeType::LeafNode) {
                    return;
                }
                traversal_.emplace(cur_node.is_right_child ? cur_node.node->GetRightChild().get()
                                                           : cur_node.node->GetLeftChild().get(),
                                   false);
            }
        }

        std::stack<StackNode> traversal_;
    };

    Iterator begin() {
        return Iterator(root_.get());
    }

    Iterator end() {
        return Iterator(root_.get(), true);
    }
};

}  // namespace algos::dd
