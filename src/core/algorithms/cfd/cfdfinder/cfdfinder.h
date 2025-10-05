#pragma once

#include <list>
#include <unordered_set>

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
#include "util/inverted_cluster_maps.h"
#include "util/pli_cache.h"

namespace algos::cfdfinder {

class CFDFinder : public Algorithm {
private:
    using Level = std::unordered_set<Candidate>;
    using Lattice = std::vector<Level>;

    config::MaxLhsType max_lhs_;
    config::ThreadNumType threads_num_ = 1;
    config::EqNullsType is_null_equal_null_ = true;
    config::InputTable input_table_;
    config::IndicesType rhs_filter_;

    Expansion expansion_strategy_ = Expansion::constant;
    Pruning pruning_strategy_ = Pruning::legacy;
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

    Lattice GetLattice(hy::PLIsPtr const& plis, hy::RowsPtr const& compressed_records);
    std::vector<std::string> GetEntriesString(
            Pattern const& pattern, InvertedClusterMaps const& inverted_cluster_maps) const;
    void EnrichCompressedRecords(hy::RowsPtr& compressed_records,
                                 EnrichedPLIs const& enriched_plis);

    std::vector<Cluster> EnrichPLI(model::PositionListIndex const* pli, int num_tuples);

    std::shared_ptr<model::PositionListIndex const> GetLhsPli(PLICache& pli_cache,
                                                              boost::dynamic_bitset<> const& lhs,
                                                              hy::PLIsPtr const& plis);

    void RegisterCFD(Candidate candidate, PatternTableau const& tableau,
                     InvertedClusterMaps const& inverted_cluster_maps);

    PatternTableau GenerateTableau(boost::dynamic_bitset<> const& lhs_attributes,
                                   model::PositionListIndex const* lhs_pli,
                                   hy::Row const& inverted_pli_rhs,
                                   hy::RowsPtr const& compressed_records_shared,
                                   ExpansionStrategy* const expansion_strategy,
                                   PruningStrategy* const pruning_strategy);

    std::list<Cluster> DetermineCover(Pattern const& child_pattern, Pattern const& current_pattern,
                                      hy::RowsPtr const& pli_records) const;

    std::unique_ptr<ExpansionStrategy> InitExpansionStrategy(
            hy::RowsPtr const& pli_records, InvertedClusterMaps const& inverted_cluster_maps);

    std::unique_ptr<PruningStrategy> InitPruningStrategy(hy::RowsPtr const& inverted_plis);

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
