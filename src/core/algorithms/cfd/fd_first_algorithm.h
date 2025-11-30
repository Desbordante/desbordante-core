#pragma once

#include "core/algorithms/cfd/cfd_discovery.h"
#include "core/algorithms/cfd/enums.h"
#include "core/algorithms/cfd/miner_node.h"
#include "core/algorithms/cfd/model/partition_tidlist.h"
#include "core/algorithms/cfd/util/prefix_tree.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

class FDFirstAlgorithm : public algos::cfd::CFDDiscovery {
    using PIdListMiners = std::vector<MinerNode<PartitionTIdList>>;
    using TIdListMiners = std::vector<MinerNode<SimpleTIdList>>;

private:
    unsigned min_supp_;
    unsigned max_cfd_size_;
    unsigned max_lhs_;
    double min_conf_;
    Substrategy substrategy_ = Substrategy::dfs;

    std::map<Itemset, PartitionTIdList> store_;
    PrefixTree<Itemset, Itemset> cand_store_;
    Itemset all_attrs_;
    std::map<std::pair<int, int>, std::vector<Itemset>> free_map_;
    std::set<Itemset> free_itemsets_;
    std::unordered_map<int, std::vector<Itemset>> rules_;

    void ResetStateCFD() final;
    void CheckForIncorrectInput() const;

    void FdsFirstDFS();
    void FdsFirstDFS(Itemset const &, std::vector<MinerNode<PartitionTIdList>> const &,
                     Substrategy = Substrategy::dfs);
    void MinePatternsBFS(Itemset const &lhs, int rhs, PartitionTIdList const &all_tids);
    void MinePatternsDFS(Itemset const &lhs, int rhs, PartitionTIdList const &all_tids);
    void MinePatternsDFS(Itemset const &, std::vector<MinerNode<SimpleTIdList>> &, Itemset const &,
                         int, RhsesPair2DList &, PartitionList &, std::vector<unsigned> &);
    std::vector<MinerNode<PartitionTIdList>> GetPartitionSingletons();

    bool Precedes(Itemset const &a, Itemset const &b);
    bool IsConstRule(PartitionTIdList const &items, int rhs_a);

    void MineFD(MinerNode<PartitionTIdList> const &inode, Itemset const &lhs, int rhs);
    std::pair<std::vector<PartitionTIdList const *>, std::vector<MinerNode<PartitionTIdList>>>
    ExpandMiningFd(MinerNode<PartitionTIdList> const &inode, int ix, Itemset const &iset,
                   std::vector<MinerNode<PartitionTIdList>> const &items) const;

    void FillMinePatternsVars(PartitionList &, RhsesPair2DList &, RuleIxs &, std::vector<int> &,
                              Itemset const &, int, PartitionTIdList const &) const;

    void AddCFDToCFDList(std::vector<int> const &sub, int out,
                         MinerNode<SimpleTIdList> const &inode, PartitionList const &partitions);

    void AnalyzeCFDFromPIdList(std::pair<int, SimpleTIdList> const &, PartitionList const &,
                               std::vector<unsigned> const &,
                               std::vector<MinerNode<SimpleTIdList>> &, Itemset const &);

    bool FillFreeMapAndItemsets(PartitionList const &partitions, Itemset const &lhs,
                                Itemset const &new_set, SimpleTIdList const &ij_tids, unsigned);

protected:
    void RegisterOptions();
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() final;

public:
    FDFirstAlgorithm();
};
}  // namespace algos::cfd
