#pragma once

#include <queue>
#include <string>
#include <vector>

#include "algorithms/near/near.h"
#include "algorithms/near/types.h"
#include "model/transaction/transactional_data.h"
#include "node.h"
#include "node_adress.h"

namespace kingfisher {

class CandidatePrefixTree {
private:
    Node root_{};
    size_t ofeat_count_;
    std::vector<OFeatureIndex> feature_frequency_order_;
    std::queue<NodeAdress> bfs_queue_;
    std::vector<model::NeARIDs> k_best_;

    struct MinCmp {
        bool operator()(model::NeARIDs const& a, model::NeARIDs const& b) const {
            return a.p_value > b.p_value;
        }
    };

    std::priority_queue<model::NeARIDs, std::vector<model::NeARIDs>, MinCmp> topk_queue_;

    double max_p_;
    unsigned max_rules_;
    double min_occurences_;
    std::shared_ptr<model::TransactionalData> transactional_data_;

    // For debug and testing
    bool save_tree_history_ = false;
    std::vector<std::string> tree_history_;

    std::optional<Node> MakeNodeFromParents(NodeAdress adress_of_node_to_make) const;
    std::optional<Node* const> GetNode(NodeAdress adress);
    std::optional<Node const* const> GetNode(NodeAdress adress) const;

    void AddChildrenToQueue(NodeAdress parent);
    void ConsiderRule(model::NeARIDs rule, Node& in_node, double parents_best);
    bool ConsPossible(NodeAdress node_addr, OConsequence cons, double best_measure) const;
    bool CheckNode(NodeAdress node);
    void CheckDepth1();
    void LapisPropagation(NodeAdress node);
    void PerformBFS();
    void FinalizeTopK();

    // For debug and testing
    void SaveTreeToHistory();
public:
    std::vector<model::NeARIDs> GetNeARIDs() const;

    // For debug and testing
    std::vector<std::string> GetTreeHistory();

    void Explore();
    CandidatePrefixTree(double max_p, unsigned max_rules,
                        std::shared_ptr<model::TransactionalData> transactional_data, bool save_tree_history = false);
    ~CandidatePrefixTree() = default;
};

}  // namespace kingfisher
