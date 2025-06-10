#include "candidate_prefix_tree.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <boost/math/distributions/hypergeometric.hpp>

#include "stats/get_fishers_p.h"
#include "stats/get_frequency.h"
#include "stats/get_lower_bounds.h"
#include "util/print_ascii_tree.h"
#include "util/vector_to_string.h"

namespace kingfisher {

std::vector<model::NeARIDs> CandidatePrefixTree::GetNeARIDs() const {
    return k_best_;
}

std::optional<Node> CandidatePrefixTree::MakeNodeFromParents(
        NodeAdress adress_of_node_to_make) const {
    Node new_node{ofeat_count_, adress_of_node_to_make.Back()};
    auto const parent_adresses = adress_of_node_to_make.GetParents();

    for (auto const& parent_adress : parent_adresses) {
        std::optional<Node const*> parent = GetNode(parent_adress);
        if (!parent.has_value()) {
            return {};
        }
        new_node.Intersect(*parent.value());
        if (new_node.Pruned()) {
            return {};
        }
    }

    return new_node;
}

std::optional<Node* const> CandidatePrefixTree::GetNode(NodeAdress adress) {
    Node* current = &root_;
    while (!adress.Empty()) {
        auto next_index = adress.PopFront();
        if (current->HasChild(next_index)) {
            current = &current->GetChild(next_index);
        } else {
            return {};
        }
    }
    return current;
}

std::optional<Node const* const> CandidatePrefixTree::GetNode(NodeAdress adress) const {
    Node const* current = &root_;
    while (!adress.Empty()) {
        auto next_index = adress.PopFront();
        if (current->HasChild(next_index)) {
            current = &current->GetChild(next_index);
        } else {
            return {};
        }
    }
    return current;
}

void CandidatePrefixTree::AddChildrenToQueue(NodeAdress adress) {
    for (NodeAdress child_adress : adress.GetChildren(ofeat_count_)) {
        bfs_queue_.emplace(child_adress);
    }
}

void CandidatePrefixTree::ConsiderRule(model::NeARIDs near, Node& in_node, double parents_best) {
    near.p_value = GetFishersP(near, transactional_data_);
    if (near.p_value >= max_p_ || near.p_value >= parents_best) {
        return;
    }
    if (topk_queue_.size() < max_rules_ && near.p_value < max_p_) {
        topk_queue_.push(std::move(near));
    } else if (near.p_value < topk_queue_.top().p_value &&
               near.p_value < in_node.p_best[near.cons.feature]) {
        topk_queue_.pop();
        topk_queue_.push(std::move(near));
        in_node.p_best[near.cons.feature] = near.p_value;
        max_p_ = near.p_value;
    }
}

bool CandidatePrefixTree::ConsPossible(NodeAdress node_addr, OConsequence cons,
                                       double best_measure) const {
    if (!node_addr.Contains(cons.feature) &&
        GetItemsetFrequency(node_addr.ToFeatures(feature_frequency_order_),
                            transactional_data_.get()) < min_occurences_) {
        std::cout << "min_occurences used" << std::endl;
        return false;
    }
    model::NeARIDs corresponding_near{node_addr.GetExceptFeat(cons.feature), cons,
                                      feature_frequency_order_};
    if (node_addr.Contains(cons.feature) &&
        GetRuleFrequency(corresponding_near, transactional_data_.get()) < min_occurences_) {
        std::cout << "min_occurences used" << std::endl;
        return false;
    }
    double lower_bound;

    unsigned kind_of_bound = 0;
    if (!node_addr.Contains(cons.feature)) {
        if (GetItemsetOccurences(node_addr.ToFeatures(feature_frequency_order_),
                                 transactional_data_.get()) <=
            GetConsMatches({feature_frequency_order_[cons.feature], cons.positive},
                           transactional_data_.get())) {
            lower_bound = GetLowerBound2(corresponding_near, transactional_data_.get());
            kind_of_bound = 2;
        } else {
            lower_bound = GetLowerBound1(feature_frequency_order_[cons.feature],
                                         transactional_data_.get());
            kind_of_bound = 1;
        }
    } else {
        lower_bound = GetLowerBound3(corresponding_near, transactional_data_.get());
        kind_of_bound = 3;
    }
    if (lower_bound > max_p_ || (lower_bound >= best_measure)) {
        std::cout << "lower_bound " << kind_of_bound << " used" << std::endl;
        return false;
    }
    return true;
}

// returns: whether the node exists after check
bool CandidatePrefixTree::CheckNode(NodeAdress node_addr) {
    // Create new node from parents on the previous layer
    OFeatureIndex adds_feature = node_addr.Back();
    auto parent_adress = node_addr.GetParent();
    auto maybe_parent_ptr = GetNode(parent_adress);
    if (!maybe_parent_ptr.has_value()) {  // direct parent was pruned
        return false;
    }
    auto parent_ptr = maybe_parent_ptr.value();

    auto maybe_node = MakeNodeFromParents(node_addr);
    if (!maybe_node.has_value()) {  // Intersection empty or one of parents was pruned
        return false;
    }
    auto& node = maybe_node.value();
    // Prune out impossible consequences in this node using lower bounds
    for (OFeatureIndex i = 0; i < ofeat_count_; i++) {
        if (node.p_possible[i])
            node.p_possible[i] = ConsPossible(node_addr, {i, true}, node.p_best[i]);
        if (node.n_possible[i])
            node.n_possible[i] = ConsPossible(node_addr, {i, false}, node.n_best[i]);
    }
    if (node.Pruned()) {
        return false;
    }
    // Evaluate rules in this node and save good ones
    for (OFeatureIndex i : node_addr.Get()) {
        if (node.p_possible[i]) {
            model::NeARIDs p_near{node_addr.GetExceptFeat(i), {i, true}, feature_frequency_order_};
            ConsiderRule(p_near, node, parent_ptr->p_best[i]);
        }
        if (node.n_possible[i]) {
            model::NeARIDs n_near{node_addr.GetExceptFeat(i), {i, false}, feature_frequency_order_};
            ConsiderRule(n_near, node, parent_ptr->n_best[i]);
        }
    }

    // Check if a minimal rule was found
    bool is_minimal = false;
    double frequency = GetItemsetFrequency(node_addr.ToFeatures(feature_frequency_order_),
                                           transactional_data_.get());
    is_minimal = (frequency == 1.0);
    if (!is_minimal) {
        for (auto parent_addr : node_addr.GetParents()) {
            if (frequency == GetItemsetFrequency(parent_addr.ToFeatures(feature_frequency_order_),
                                                 transactional_data_.get())) {
                is_minimal = true;
                break;
            }
        }
    }
    // Prune out consequences in this node by minimality
    if (is_minimal) {
        for (OFeatureIndex i = 0; i < ofeat_count_; i++) {
            if (node_addr.Contains(i)) {
                continue;
            }
            node.p_possible[i] = false;
            node.n_possible[i] = false;
        }
    }

    if (node.Pruned()) {
        return false;
    }

    // Prune out consequences in parents using Lapis Philosophorum principle from paper
    // Optimization possible: only call Propagate for features that were set impossible during this
    // CheckNode call.
    for (OFeatureIndex feat = 0; feat < ofeat_count_; ++feat) {
        auto parent_adress = NodeAdress(node_addr.GetExcept(feat));
        double parent_frequency = GetItemsetFrequency(
                parent_adress.ToFeatures(feature_frequency_order_), transactional_data_.get());
        model::NeARIDs pos_near{
                node_addr.GetExceptFeat(feat), {feat, true}, feature_frequency_order_};
        model::NeARIDs neg_near{
                node_addr.GetExceptFeat(feat), {feat, false}, feature_frequency_order_};
        double pos_near_frequency = GetRuleFrequency(pos_near, transactional_data_.get());
        double neg_near_frequency = GetRuleFrequency(neg_near, transactional_data_.get());

        double positive_conditional_freq = pos_near_frequency / parent_frequency;
        double negative_conditional_freq = neg_near_frequency / parent_frequency;
        if (positive_conditional_freq == 1.0 || negative_conditional_freq == 1.0) {
            // By minimality:
            node.p_possible[feat] = false;
            node.n_possible[feat] = false;
            // By Lapis Philosophorum (Propagation):
            auto maybe_parent_ptr = GetNode(parent_adress);
            if (maybe_parent_ptr.has_value()) {
                auto parent_ptr = maybe_parent_ptr.value();
                parent_ptr->p_possible[feat] = false;
                parent_ptr->n_possible[feat] = false;
            }
        }
    }
    // Node wasn't pruned: add it to tree
    auto node_ptr = std::make_shared<Node>(std::move(node));
    parent_ptr->AddChild(adds_feature, node_ptr);
    return true;
}

void CandidatePrefixTree::CheckDepth1() {
    for (auto& [node_feat, node_ptr] : root_.children) {
        for (OFeatureIndex i = 0; i < ofeat_count_; i++) {
            node_ptr->p_possible[i] = ConsPossible({node_feat}, {i, true}, node_ptr->p_best[i]);
            node_ptr->n_possible[i] = ConsPossible({node_feat}, {i, false}, node_ptr->n_best[i]);
        }
    }
}

// Called Lapis Philosophorum in the paper
void CandidatePrefixTree::LapisPropagation(NodeAdress node_addr) {
    for (OFeatureIndex except_feat = 0; except_feat < ofeat_count_; ++except_feat) {
        auto parent_adress = NodeAdress(node_addr.GetExcept(except_feat));
        auto maybe_parent_ptr = GetNode(parent_adress);
        if (maybe_parent_ptr.has_value()) {
            auto parent_ptr = maybe_parent_ptr.value();
            parent_ptr->p_possible[except_feat] = false;
            parent_ptr->n_possible[except_feat] = false;
        }
    }
}

void CandidatePrefixTree::PerformBFS() {
    for (auto& [depth1_node_feat, depth1_node_ptr] : root_.children) {
        NodeAdress depth1_node_addr{depth1_node_feat};
        AddChildrenToQueue(std::move(depth1_node_addr));  // Initialize queue with depth 2 nodes
        deletion_queue_.emplace(std::move(depth1_node_addr));
    }
    unsigned current_depth_ = 2;
    while (bfs_queue_.size() != 0) {
        if (bfs_queue_.front().Size() > current_depth_) {
            current_depth_ = bfs_queue_.front().Size();
            unsigned grandparent_depth = current_depth_ - 2;
            NodeAdress leftmost_grandparent_addr{
                    {std::vector<OFeatureIndex>(grandparent_depth, 0)}};
            do {
                auto maybe_grandparent = GetNode(leftmost_grandparent_addr);
                if (maybe_grandparent.has_value()) {
                    maybe_grandparent.value()->Clear();
                }
            } while (leftmost_grandparent_addr.Increment(ofeat_count_));
        }
        if (CheckNode(bfs_queue_.front()) == false) {
            LapisPropagation(bfs_queue_.front());
        }
        AddChildrenToQueue(bfs_queue_.front());
        std::cout << "\n_____checked " + bfs_queue_.front().ToString() + "________\n";
        deletion_queue_.emplace(std::move(bfs_queue_.front()));
        bfs_queue_.pop();
        PrintAsciiTree(root_, ofeat_count_);  // DEBUG
    }
}

void CandidatePrefixTree::FinalizeTopK() {
    k_best_.clear();
    k_best_.reserve(topk_queue_.size());
    while (!topk_queue_.empty()) {
        k_best_.push_back(std::move(topk_queue_.top()));
        topk_queue_.pop();
    }
    std::sort(k_best_.begin(), k_best_.end(),
              [](auto const& A, auto const& B) { return A.p_value < B.p_value; });
}

std::string CandidatePrefixTree::GetTreeHistory() {
    return tree_history_;
}

void CandidatePrefixTree::Explore() {
    CheckDepth1();
    PerformBFS();
    FinalizeTopK();
}

CandidatePrefixTree::CandidatePrefixTree(
        double max_p, unsigned max_rules,
        std::shared_ptr<model::TransactionalData> transactional_data,
        bool save_tree_history)
    : max_p_(max_p),
      max_rules_(max_rules),
      transactional_data_(transactional_data),
      save_tree_history_(save_tree_history) {
    min_occurences_ = GetMinOccurences(max_p_, transactional_data_.get());
    feature_frequency_order_ = GetFeatureFrequencyOrder(min_occurences_, transactional_data_.get());
    ofeat_count_ = feature_frequency_order_.size();
    for (size_t feat = 0; feat < ofeat_count_; ++feat) {
        auto node = Node(ofeat_count_, OFeatureIndex(feat));
        root_.AddChild(OFeatureIndex(feat), std::make_shared<Node>(std::move(node)));
    }
}

}  // namespace kingfisher
