#pragma once

#include <list>
#include <memory>
#include <set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/cfd/cfdfinder/cfd.h"
#include "core/algorithms/cfd/cfdfinder/enums.h"
#include "core/algorithms/cfd/cfdfinder/model/cfdfinder_relation_data.h"
#include "core/algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/result/result_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/enriched_plis.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"
#include "core/algorithms/cfd/cfdfinder/types/inverted_cluster_maps.h"
#include "core/algorithms/cfd/cfdfinder/util/pli_cache.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/indices/type.h"
#include "core/config/max_lhs/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/config/thread_number/type.h"

namespace algos::cfdfinder {

class CFDFinder : public Algorithm {
private:
    using Level = std::set<Candidate>;
    using Lattice = std::vector<Level>;

    config::EqNullsType is_null_equal_null_ = true;
    config::InputTable input_table_;

    config::MaxLhsType max_lhs_;
    config::ThreadNumType threads_num_ = 1;
    config::IndicesType rhs_filter_;

    Expansion expansion_strategy_ = Expansion::kConstant;
    Pruning pruning_strategy_ = Pruning::kLegacy;
    Result result_strategy_ = Result::kLattice;
    double min_confidence_;
    double min_support_;
    double max_g1_;
    double min_support_gain_;
    double max_level_support_drop_;
    unsigned int max_patterns_;
    unsigned int limit_pli_cache_;

    std::list<CFD> cfd_collection_;
    std::shared_ptr<RelationalSchema const> schema_;
    std::unique_ptr<CFDFinderRelationData> relation_;
    PLIsPtr plis_;
    ColumnsPtr inverted_plis_;
    RowsPtr compressed_records_;

    std::shared_ptr<ExpansionStrategy> InitExpansionStrategy(
            RowsPtr pli_records, InvertedClusterMaps const& inverted_cluster_maps);
    std::shared_ptr<PruningStrategy> InitPruningStrategy(ColumnsPtr inverted_plis);
    std::shared_ptr<ResultStrategy> InitResultStrategy();

    Lattice GetLattice();
    void EnrichCompressedRecords(EnrichedPLIs enriched_plis) const;
    InvertedClusterMaps BuildEnrichedStructures() const;

    std::list<Candidate> RunHyFdPhase() const;

    PatternTableau GenerateTableau(boost::dynamic_bitset<> const& lhs_attributes,
                                   model::PLI const* lhs_pli, Row const& inverted_pli_rhs,
                                   std::shared_ptr<ExpansionStrategy> expansion_strategy,
                                   std::shared_ptr<PruningStrategy> pruning_strategy);
    void RegisterResults(std::list<RawCFD> results, InvertedClusterMaps inverted_cluster_maps);

    void TraverseLatticeSeq(InvertedClusterMaps inverted_cluster_maps, Lattice&& levels,
                            PLICache& pli_cache);
    void TraverseLatticePar(InvertedClusterMaps inverted_cluster_maps, Lattice&& levels,
                            PLICache& pli_cache);

    void RegisterOptions();

    void ResetState() final {
        cfd_collection_.clear();
        plis_.reset();
        inverted_plis_.reset();
        compressed_records_.reset();
    };

protected:
    void MakeExecuteOptsAvailable() override;
    void LoadDataInternal() final;
    void ExecuteInternal() final;

public:
    CFDFinder();

    std::list<CFD> const& CfdList() const noexcept {
        return cfd_collection_;
    }
};

}  // namespace algos::cfdfinder
