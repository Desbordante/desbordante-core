#include "SearchTree.h"

SearchTree::Node::Node(size_t bit, SearchTree::Bitset set, SearchTree::Bitset sets_union, SearchTree::Bitset sets_inter,
                       const std::shared_ptr<Node>& parent, std::shared_ptr<Node> left, std::shared_ptr<Node> right)
        : bit_(bit), set_(std::move(set)), union_(std::move(sets_union)),
          inter_(std::move(sets_inter)), left_(std::move(left)),
          right_(std::move(right)), parent_(parent){}

SearchTree::Node::Node(size_t bit, SearchTree::Bitset sets_union, SearchTree::Bitset sets_inter,
                       const std::shared_ptr<Node>& parent,
                       std::shared_ptr<Node> left, std::shared_ptr<Node> right)
        : bit_(bit), union_(std::move(sets_union)),
          inter_(std::move(sets_inter)), left_(std::move(left)),
          right_(std::move(right)), parent_(parent){}

SearchTree::Node::Node(size_t bit, SearchTree::Bitset set, const std::shared_ptr<Node>& parent)
        :bit_(bit), set_(std::move(set)), parent_(parent){}

bool SearchTree::Node::IsLeaf() const {
    assert((left_ != nullptr) ^ (right_ == nullptr));
    return !left_ && !right_;
}

SearchTree::Bitset SearchTree::Node::GetUnion() const {
    return IsLeaf()? set_ : union_;
}

SearchTree::Bitset SearchTree::Node::GetInter() const {
    return IsLeaf()? set_ : inter_;
}

SearchTree::SearchTree(size_t number_of_attributes)
        :number_of_attributes_(number_of_attributes){}

SearchTree::SearchTree(const Bitset& set)
        :number_of_attributes_(set.size()){
    CreateSingleElementSets(set);
}

void SearchTree::CreateSingleElementSets(const Bitset& set) {
    for (size_t bit = set.find_first(); bit != Bitset::npos; bit = set.find_next(bit)){
        Bitset to_add(number_of_attributes_);
        to_add.set(bit);
        Add(to_add);
    }
}

bool SearchTree::Add(const Bitset& set) {
    assert(!set.empty());
    assert(set.size() == number_of_attributes_);

    if (!root_){
        root_ = std::make_shared<Node>(set.find_first(), set, nullptr);
        ++cardinality_;
        return true;
    }

    auto current_node = root_;
    for (size_t prev_node_bit = 0, curr_node_bit = current_node->bit_;
         !current_node->IsLeaf();
         current_node = set[curr_node_bit] ? current_node->right_ : current_node->left_,
                prev_node_bit = curr_node_bit + 1, curr_node_bit = current_node->bit_){
        assert(curr_node_bit != Bitset::npos);
        assert(prev_node_bit <= curr_node_bit);
        for (size_t set_bit = prev_node_bit; set_bit < curr_node_bit; ++set_bit){
            bool in_set = set[set_bit];
            if (in_set != current_node->union_[set_bit]){
                auto new_left = std::make_shared<Node>(
                    curr_node_bit, current_node->set_,
                    current_node->union_, current_node->inter_,
                    current_node, current_node->left_, current_node->right_);
                auto new_right = std::make_shared<Node>(
                    set.find_next(set_bit), set, current_node);
                auto replacing_node = new_left;
                if (!in_set){
                    std::swap(new_left, new_right);
                    replacing_node = new_right;
                }

                current_node->left_->parent_ = replacing_node;
                current_node->right_->parent_ = replacing_node;

                current_node->bit_ = set_bit;
                current_node->left_ = new_left;
                current_node->right_ = new_right;

                UpdateInterAndUnion(current_node);
                ++cardinality_;
                return true;
            }
        }
    }

    auto node_set = current_node->set_;
    size_t node_bit, set_bit;
    for (node_bit = node_set.find_first(), set_bit = set.find_first();
    node_bit == set_bit && node_bit != Bitset::npos;
    node_bit = node_set.find_next(node_bit), set_bit = set.find_next(set_bit));
    if (node_bit == set_bit){
        return false;
    }

    auto new_left = std::make_shared<Node>(node_bit, node_set, current_node);
    auto new_right = std::make_shared<Node>(set_bit, set, current_node);

    if (node_bit < set_bit){
        new_left->bit_ = node_set.find_next(node_bit);
        std::swap(new_left, new_right);
    } else {
        new_right->bit_ = set.find_next(set_bit);
    }

    current_node->bit_ = std::min(node_bit, set_bit);
    current_node->left_ = new_left;
    current_node->right_ = new_right;

    UpdateInterAndUnion(current_node);
    ++cardinality_;
    return true;
}

bool SearchTree::Remove(const Bitset& set) {
    assert(!set.empty());
    if (!root_){
        return false;
    }

    auto current_node = root_;
    for (size_t set_bit = set.find_first(), node_bit = current_node->bit_;
         !current_node->IsLeaf();
           node_bit = current_node->bit_){
        if (!(set.is_subset_of(current_node->union_)
        && current_node->inter_.is_subset_of(set))){
            return false;
        }

        if (set[node_bit]){
            current_node = current_node->right_;
            set_bit = set.find_next(set_bit);
        } else {
            current_node = current_node->left_;
        }
    }

    if (current_node->set_ != set){
        return false;
    }

    if (current_node == root_){
        root_ = nullptr;
        --cardinality_;
        return true;
    }

    auto parent_node = current_node->parent_.lock();
    auto another_child_node = (parent_node->right_ == current_node)
                                  ? parent_node->left_
                                  : parent_node->right_;

    parent_node->left_ = another_child_node->left_;
    parent_node->right_ = another_child_node->right_;
    parent_node->union_ = another_child_node->union_;
    parent_node->inter_ = another_child_node->inter_;
    if (!another_child_node->IsLeaf()){
        parent_node->bit_ = another_child_node->bit_;
        parent_node->left_->parent_ = parent_node;
        parent_node->right_->parent_ = parent_node;
    }

    UpdateInterAndUnion(parent_node->parent_.lock());
    --cardinality_;
    return true;
}

void SearchTree::CollectSubsets(const Bitset& set, const std::shared_ptr<Node>& current_node,
                                const std::function<void(const Bitset& )>& collect, bool& go_further) const {
    if(current_node->IsLeaf()){
        auto node_set = current_node->set_;
        if (node_set.is_subset_of(set)){
            collect(node_set);
        }
        return;
    }

    if (go_further && current_node->inter_.is_subset_of(set)){
        CollectSubsets(set, current_node->left_, collect, go_further);
        if (set[current_node->bit_]){
            CollectSubsets(set, current_node->right_, collect, go_further);
        }
    }
}

bool SearchTree::ContainsAnySubsetOf(const Bitset& set) const{
    bool go_further = true;
    auto contains_func = [&go_further](const Bitset& ){
        go_further = false;
    };

    if (root_){
        CollectSubsets(set, root_, contains_func, go_further);
    }

    return !go_further;
}

void SearchTree::ForEach(const std::shared_ptr<Node>& current_node,
                         const std::function<void(const Bitset& )>& collect) const {
    if (current_node->IsLeaf()){
        collect(current_node->set_);
        return;
    }

    ForEach(current_node->left_, collect);
    ForEach(current_node->right_, collect);
}

void SearchTree::ForEach(const std::function<void(const Bitset& )>& collect) const {
    if (root_){
        ForEach(root_, collect);
    }
}

void SearchTree::ForEachSubset(const SearchTree::Bitset& set,
                               const std::function<void(const Bitset& )>& collect) const {
    if (root_){
        bool go_further = true;
        CollectSubsets(set, root_, collect, go_further);
    }
}

void SearchTree::UpdateInterAndUnion(const std::shared_ptr<Node>& node) {
    auto node_copy = node;
    while (node_copy){
        auto left = node_copy->left_;
        auto right = node_copy->right_;
        node_copy->union_ = left->GetUnion() | right->GetUnion();
        node_copy->inter_ = left->GetInter() & right->GetInter();
        node_copy = node_copy->parent_.lock();
    }
}