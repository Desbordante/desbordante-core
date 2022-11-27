#pragma once

#include <list>
#include <queue>
#include <stack>
#include <vector>

#include "algorithms/ar_algorithm.h"
#include "algorithms/association_rules/candidate_hash_tree.h"
#include "algorithms/association_rules/node.h"
#include "model/itemset.h"

namespace algos {

class Apriori : public ARAlgorithm {
private:
    // TODO(alexandrsmirn): попробовать убрать из полей, и создавать просто в методе GenerateAllRules
    std::unique_ptr<CandidateHashTree> candidate_hash_tree_;

    Node root_;
    std::unordered_map<Node*, std::list<Node>> candidates_;
    unsigned level_num_ = 1;

    bool GenerateNextCandidateLevel();

    bool CanBePruned(std::vector<unsigned> const& itemset);
    void GenerateCandidates(std::vector<Node>& children);
    void CreateFirstLevelCandidates();
    void AppendToTree();

    static void UpdatePath(std::stack<Node*>& path, std::vector<Node>& vertices);
    static void UpdatePath(std::queue<Node const*>& path, std::vector<Node> const& vertices);
    static void UpdatePath(std::stack<Node const*>& path, std::vector<Node> const& vertices);

    double GetSupport(std::vector<unsigned> const& frequent_itemset) const override;
    unsigned long long GenerateAllRules() override;
    unsigned long long FindFrequent() override;

public:
    Apriori();

    std::list<std::set<std::string>> GetFrequentList() const override;
};

} // namespace algos
