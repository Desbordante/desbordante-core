#pragma once

// see ./LICENSE

#include <boost/any.hpp>
#include <filesystem>
#include <list>
#include <map>
#include <mutex>
#include <vector>

#include "algorithms/options/type.h"
#include "model/fd.h"
#include "model/cfd.h"
#include "util/prefix_tree.h"
#include "algorithms/primitive.h"
#include "cfd_relation_data.h"
#include "generator_store.h"
#include "miner_node.h"
#include "partition_table.h"


namespace algos {


enum SUBSTRATEGY { kDfs, kBfs };

class CFDDiscovery : public algos::Primitive {
private:
    unsigned min_supp_;
    unsigned max_cfd_size_;
    unsigned max_lhs_ = 0;
    double min_conf_;
    std::shared_ptr<CFDRelationData> relation_;
    CFDList cfd_list_;
    unsigned columns_number_;
    unsigned tuples_number_;
    bool is_null_equal_null_;
    std::string algo_name_;

    static const config::OptionType<decltype(min_supp_)> MinSupportOpt;
    static const config::OptionType<decltype(columns_number_)> ColumnsNumberOpt;
    static const config::OptionType<decltype(tuples_number_)> TuplesNumberOpt;
    static const config::OptionType<decltype(min_conf_)> MinConfidenceOpt;
    static const config::OptionType<decltype(max_lhs_)> MaxLhsSizeOpt;
    //static const config::OptionType<decltype(algo_name_)> AlgoStrOpt;
protected:

    unsigned long long ExecuteInternal() final;
    void MakeExecuteOptsAvailable() final;

public:
    constexpr static std::string_view kDefaultPhaseName = "CFD mining";
    explicit CFDDiscovery(std::vector<std::string_view> phase_names);
    explicit CFDDiscovery();
    void FitInternal(model::IDatasetStream& data_stream) final;

    int NrCfds() const;
    CFDList GetCfds() const;
    std::string GetRelationString(char delim = ' ') const;
    std::string GetRelationString(const SimpleTidList& subset, char delim= ' ') const;
    std::string GetCfdString(CFD const& cfd) const;
    /* void Ctane(int min_supp, unsigned max_size, double conf = 1);

    void IntegratedDFS(int min_supp, unsigned max_size, double conf = 1);
    void IntegratedDFS(const Itemset &, std::vector<MinerNode<PartitionTidList> > &);

    void ItemsetsFirstBFS(int min_supp, unsigned max_size, SUBSTRATEGY= SUBSTRATEGY::kBfs, double min_conf = 1);
    void ItemsetsFirstDFS(int min_supp, unsigned max_size, SUBSTRATEGY= SUBSTRATEGY::kBfs, double min_conf = 1);
    void ItemsetsFirstDFS(const Itemset &, std::vector<MinerNode<SimpleTidList> > &, SUBSTRATEGY= SUBSTRATEGY::kBfs);
    void MineFDs(const SimpleTidList&, const Itemset&, const Itemset&);
    void MineFDsDFS(const SimpleTidList &, const Itemset &, const Itemset &);
    void MineFDsDFS(std::vector<MinerNode<PartitionTidList> > &, const Itemset &, const Itemset &);

    void FdsFirstBFS(int min_supp, unsigned max_size, SUBSTRATEGY= SUBSTRATEGY::kBfs, double min_conf = 1); */
    void FdsFirstDFS(int min_supp, unsigned max_lhs_size, SUBSTRATEGY= SUBSTRATEGY::kBfs, double min_conf = 1);
    void FdsFirstDFS(const Itemset &, std::vector<MinerNode<PartitionTidList> > &, SUBSTRATEGY= SUBSTRATEGY::kBfs);
    // void MinePatternsBFS(const Itemset &lhs, int rhs, const PartitionTidList &all_tids);
    void MinePatternsDFS(const Itemset &lhs, int rhs, const PartitionTidList &all_tids);
    void MinePatternsDFS(const Itemset &, std::vector<MinerNode<SimpleTidList> > &, const Itemset &, int,
                         std::vector<std::vector<std::pair<int, int> > > &,
                         std::vector<std::pair<Itemset, std::vector<int> > > &, std::vector<int> &);

    std::vector<MinerNode<PartitionTidList> > GetPartitionSingletons();
    // Unused for fd-first
    std::vector<MinerNode<PartitionTidList> > GetPartitionSingletons(const SimpleTidList&, const Itemset&);
    // Unused for fd-first
    std::vector<MinerNode<PartitionTidList> > GetAllSingletons(int);
    std::vector<MinerNode<SimpleTidList> > GetSingletons(int);

protected:
    void RegisterOptions();
    bool Precedes(const Itemset& a, const Itemset& b);
    // void PruneCands(std::vector<MinerNode<PartitionTidList> >& items, const Itemset& sub, int out);
    // void PruneCands(std::vector<MinerNode<SimpleTidList> >& items, const Itemset& sub, int out);
    bool IsConstRule(const PartitionTidList& items, int rhs_a);
    bool IsConstRulePartition(const SimpleTidList &items, const std::vector<std::vector<std::pair<int, int> > > &rhses);
    int GetPartitionSupport(const SimpleTidList& pids, const std::vector<int>& partitions);
    int GetPartitionError(const SimpleTidList& pids, const std::vector<std::pair<Itemset, std::vector<int> > >& partitions);

private:

    GeneratorStore<int> generators_;
    std::map<Itemset,PartitionTidList> store_;
    std::unordered_map<Itemset,SimpleTidList> tid_store_;
    PrefixTree<Itemset, Itemset> cand_store_;
    Itemset all_attrs_;
    std::vector<MinerNode<SimpleTidList> > items_layer_;
    std::map<std::pair<int,int>, std::vector<Itemset> > free_map_;
    // Пока хз что значит
    std::set<Itemset> free_itemsets_;
    std::unordered_map<int, std::vector<Itemset> > rules_;
    Itemset attribute_order_;
};
}