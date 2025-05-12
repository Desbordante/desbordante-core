#pragma once

#include <memory>    // for shared_ptr
#include <stddef.h>  // for size_t
#include <vector>    // for vector

#include "algorithms/nar/nar_algorithm.h"             // for NARAlgorithm
#include "differential_functions.h"                   // for DifferentialOpt...
#include "encoded_nar.h"                              // for EncodedNAR
#include "nar/value_range.h"                          // for ValueRange
#include "rng.h"                                      // for RNG
#include "table/column_layout_typed_relation_data.h"  // for ColumnLayoutTyp...

namespace algos::des {
using FeatureDomains = std::vector<std::shared_ptr<model::ValueRange>> const;
using TypedRelation = model::ColumnLayoutTypedRelationData;

class DES : public NARAlgorithm {
private:
    long unsigned int seed_;
    unsigned int population_size_;
    unsigned int num_evaluations_;
    DifferentialOptions differential_options_;
    RNG rng_;
    void RegisterOptions();

    static FeatureDomains FindFeatureDomains(TypedRelation const* typed_relation);
    std::vector<EncodedNAR> GetRandomPopulationInDomains(FeatureDomains const& domains,
                                                         RNG& rng) const;
    EncodedNAR MutatedIndividual(std::vector<EncodedNAR> const& population, size_t at,
                                 RNG& rng) const;

protected:
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    DES();
};

}  // namespace algos::des
