//
// Created by mikhail on 14.03.23.
//
#pragma once

#include "algorithms/cfd/cfd_discovery.h"
#include "algorithms/cfd/enums.h"
#include "algorithms/cfd/miner_node.h"
#include "algorithms/cfd/util/prefix_tree.h"

namespace algos {

class FDFirstAlgorithm : public algos::CFDDiscovery {
private:
    unsigned min_supp_;
    unsigned max_cfd_size_;
    unsigned max_lhs_;
    double min_conf_;
    Substrategy substrategy_ = Substrategy::dfs;

    std::map<Itemset,PartitionTidList> store_;
    PrefixTree<Itemset, Itemset> cand_store_;
    Itemset all_attrs_;
    std::map<std::pair<int,int>, std::vector<Itemset> > free_map_;
    std::set<Itemset> free_itemsets_;
    std::unordered_map<int, std::vector<Itemset> > rules_;

    void ResetStateCFD() final;

    void FdsFirstDFS(int min_supp, unsigned max_lhs_size, Substrategy = Substrategy::dfs, double min_conf = 1);
    void FdsFirstDFS(const Itemset &, std::vector<MinerNode<PartitionTidList>> &,
                     Substrategy = Substrategy::dfs);
    void MinePatternsBFS(const Itemset &lhs, int rhs, const PartitionTidList &all_tids);
    void MinePatternsDFS(const Itemset &lhs, int rhs, const PartitionTidList &all_tids);
    void MinePatternsDFS(const Itemset &, std::vector<MinerNode<SimpleTidList>> &, const Itemset &, int,
                         std::vector<std::vector<std::pair<int, int> > > &,
                         std::vector<std::pair<Itemset, std::vector<int> > > &, std::vector<int> &);
    std::vector<MinerNode<PartitionTidList> > GetPartitionSingletons();

    bool Precedes(const Itemset& a, const Itemset& b);
    bool IsConstRule(const PartitionTidList& items, int rhs_a);
    bool IsConstRulePartition(const SimpleTidList &items, const std::vector<std::vector<std::pair<int, int> > > &rhses);
    int GetPartitionSupport(const SimpleTidList& pids, const std::vector<int>& partitions);
    int GetPartitionError(const SimpleTidList& pids, const std::vector<std::pair<Itemset, std::vector<int> > >& partitions);

    void CheckForIncorrectInput() const;
protected:
    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() final;
public:
    explicit FDFirstAlgorithm(std::vector<std::string_view> phase_names);
    explicit FDFirstAlgorithm();

};

}