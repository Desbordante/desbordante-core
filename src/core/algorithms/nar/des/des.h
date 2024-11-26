#pragma once

#include "algorithms/nar/nar_algorithm.h"
#include "config/names.h"
#include "differential_functions.h"
#include "encoded_nar.h"
#include "enums.h"

namespace algos::des {
using FeatureDomains = std::vector<std::shared_ptr<model::ValueRange>> const;
using TypedRelation = model::ColumnLayoutTypedRelationData;

class DES : public NARAlgorithm {
private:
    int population_size_;
    int num_evaluations_;
    DifferentialOptions differential_options_;

    void RegisterOptions();

    static FeatureDomains FindFeatureDomains(TypedRelation const* typed_relation);
    std::vector<EncodedNAR> GetRandomPopulationInDomains(FeatureDomains const& domains) const;
    void EvolvePopulation(std::vector<EncodedNAR>& population) const;
    EncodedNAR MutateIndividual(std::vector<EncodedNAR> population, size_t at);

protected:
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    DES();
};

}  // namespace algos::des
