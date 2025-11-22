#pragma once

#include <list>
#include <memory>
#include <set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "algorithms/algorithm.h"
#include "algorithms/cfd/cfdfinder/candidate.h"
#include "algorithms/cfd/cfdfinder/cfd.h"
#include "algorithms/cfd/cfdfinder/enums.h"
#include "algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "algorithms/cfd/cfdfinder/model/result/result_strategy.h"
#include "algorithms/cfd/cfdfinder/types/enriched_plis.h"
#include "algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "algorithms/cfd/cfdfinder/util/pli_cache.h"
#include "config/equal_nulls/type.h"
#include "config/indices/type.h"
#include "config/max_lhs/type.h"
#include "config/tabular_data/input_table_type.h"
#include "config/thread_number/type.h"

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

    Lattice GetLattice(PLIsPtr plis, RowsPtr compressed_records);
    void EnrichCompressedRecords(RowsPtr compressed_records, EnrichedPLIs enriched_plis) const;

    std::vector<Cluster> EnrichPLI(model::PLI const* pli, int num_tuples) const;

    std::shared_ptr<model::PLI const> GetLhsPli(PLICache& pli_cache,
                                                boost::dynamic_bitset<> const& lhs,
                                                PLIs const& plis);

    PatternTableau GenerateTableau(boost::dynamic_bitset<> const& lhs_attributes,
                                   model::PLI const* lhs_pli, Row const& inverted_pli_rhs,
                                   RowsPtr compressed_records_shared,
                                   std::shared_ptr<ExpansionStrategy> expansion_strategy,
                                   std::shared_ptr<PruningStrategy> pruning_strategy);

    std::list<Cluster> DetermineCover(Pattern const& child_pattern, Pattern const& current_pattern,
                                      Rows const& pli_records) const;

    std::shared_ptr<ExpansionStrategy> InitExpansionStrategy(
            RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps);
    std::shared_ptr<PruningStrategy> InitPruningStrategy(ColumnsPtr inverted_plis);
    std::shared_ptr<ResultStrategy> InitResultStrategy();
    InvertedClusterMaps BuildEnrichedStructures(PLIsPtr plis_shared,
                                                RowsPtr compressed_records_shared) const;
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
