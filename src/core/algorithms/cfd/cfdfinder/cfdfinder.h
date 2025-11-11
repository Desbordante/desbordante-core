#pragma once

#include <list>
#include <set>

#include "algorithms/algorithm.h"
#include "candidate.h"
#include "cfd.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/max_lhs/type.h"
#include "config/tabular_data/input_table_type.h"
#include "config/thread_number/type.h"
#include "enums.h"
#include "model/cfdfinder_relation_data.h"
#include "model/expansion/expansion_strategy.h"
#include "model/pruning/pruning_strategy.h"
#include "model/result/result_strategy.h"
#include "types/bitset.h"
#include "types/inverted_cluster_maps.h"
#include "util/pli_cache.h"

namespace algos::cfdfinder {

class CFDFinder : public Algorithm {
private:
    using Level = std::set<Candidate>;
    using Lattice = std::vector<Level>;

    config::MaxLhsType max_lhs_;
    config::ThreadNumType threads_num_ = 1;
    config::EqNullsType is_null_equal_null_ = true;
    config::InputTable input_table_;
    config::IndicesType rhs_filter_;

    Expansion expansion_strategy_ = Expansion::constant;
    Pruning pruning_strategy_ = Pruning::legacy;
    Result result_strategy_ = Result::lattice;

    double min_confidence_;
    double min_support_;
    double max_g1_;
    double min_support_gain_;
    double max_level_support_drop_;
    unsigned int max_patterns_;
    unsigned int limit_pli_cache_;

    std::list<CFD> cfd_collection_;
    std::unique_ptr<CFDFinderRelationData> relation_;

    void RegisterOptions();
    void ResetState() final;

    Lattice GetLattice(hy::PLIsPtr plis, hy::RowsPtr compressed_records);
    void EnrichCompressedRecords(hy::RowsPtr compressed_records, EnrichedPLIs enriched_plis);

    std::vector<Cluster> EnrichPLI(model::PLI const* pli, int num_tuples);

    std::shared_ptr<model::PLI const> GetLhsPli(PLICache& pli_cache, BitSet const& lhs,
                                                hy::PLIs const& plis);

    PatternTableau GenerateTableau(BitSet const& lhs_attributes, model::PLI const* lhs_pli,
                                   hy::Row const& inverted_pli_rhs,
                                   hy::RowsPtr compressed_records_shared,
                                   std::shared_ptr<ExpansionStrategy> expansion_strategy,
                                   std::shared_ptr<PruningStrategy> pruning_strategy);

    std::list<Cluster> DetermineCover(Pattern const& child_pattern, Pattern const& current_pattern,
                                      hy::Rows const& pli_records) const;

    std::shared_ptr<ExpansionStrategy> InitExpansionStrategy(
            hy::RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps);
    std::shared_ptr<PruningStrategy> InitPruningStrategy(hy::RowsPtr inverted_plis);
    std::shared_ptr<ResultStrategy> InitResultStrategy();

    void RegisterResults(std::shared_ptr<ResultStrategy> result_receiver,
                         InvertedClusterMaps inverted_cluster_maps);

protected:
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;

public:
    CFDFinder();

    std::list<CFD> const& CfdList() const noexcept {
        return cfd_collection_;
    }
};

}  // namespace algos::cfdfinder
