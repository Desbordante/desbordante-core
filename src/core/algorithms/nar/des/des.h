#pragma once

#include "core/algorithms/nar/des/differential_functions.h"
#include "core/algorithms/nar/des/encoded_nar.h"
#include "core/algorithms/nar/des/enums.h"
#include "core/algorithms/nar/des/rng.h"
#include "core/algorithms/nar/nar_algorithm.h"
#include "core/config/names.h"

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
