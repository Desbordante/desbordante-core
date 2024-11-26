#include "des.h"

#include "algorithms/nar/value_range.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "model/types/types.h"

namespace algos::des {
using model::ValueRange;

DES::DES() : NARAlgorithm({}) {
    using namespace config::names;
    RegisterOptions();
}

void DES::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    DifferentialStrategy default_strategy = DifferentialStrategy::rand1Bin;
    RegisterOption(Option{&population_size_, kPopulationSize, kDPopulationSize, 100});
    RegisterOption(
            Option{&num_evaluations_, kMaxFitnessEvaluations, kDMaxFitnessEvaluations, 1000});
    RegisterOption(Option{&differential_options_.differential_scale, kDifferentialScale,
                          kDDifferentialScale, 0.5});
    RegisterOption(Option{&differential_options_.crossover_probability, kCrossoverProbability,
                          kDCrossoverProbability, 0.9});
    RegisterOption(Option{&differential_options_.differential_strategy, kDifferentialStrategy,
                          kDDifferentialStrategy, default_strategy});
}

void DES::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kPopulationSize, kMaxFitnessEvaluations, kDifferentialScale,
                          kCrossoverProbability, kDifferentialStrategy});
}

FeatureDomains DES::FindFeatureDomains(TypedRelation const* typed_relation) {
    std::vector<std::shared_ptr<ValueRange>> feature_domains;
    feature_domains.reserve(typed_relation->GetNumColumns());
    for (size_t i = 0; i < typed_relation->GetNumColumns(); i++) {
        std::shared_ptr<ValueRange> domain = CreateValueRange(typed_relation->GetColumnData(i));
        feature_domains.push_back(std::move(domain));
    }
    return feature_domains;
}

std::vector<EncodedNAR> DES::GetRandomPopulationInDomains(FeatureDomains domains) const {
    auto population = std::vector<EncodedNAR>();
    for (int i = 0; i < population_size_; i++) {
        population.emplace_back(EncodedNAR(domains, typed_relation_.get()));
    }
    auto compare_by_fitness = [](EncodedNAR const& a, EncodedNAR const& b) -> bool {
        return a.GetQualities().fitness > b.GetQualities().fitness;
    };
    std::sort(population.begin(), population.end(), compare_by_fitness);
    return population;
}

EncodedNAR DES::MutateIndividual(std::vector<EncodedNAR> population, size_t at) {
    MutationFunction diff_func =
            EnumToMutationStrategy(differential_options_.differential_strategy);
    return (*diff_func)(population, at, differential_options_);
}

unsigned long long DES::ExecuteInternal() {
    FeatureDomains feature_domains = FindFeatureDomains(typed_relation_.get());
    std::vector<EncodedNAR> population = GetRandomPopulationInDomains(feature_domains);

    for (int i = 0; i < num_evaluations_;
         i++) {  // TODO: change num_evaluations type to something unsigned
        size_t candidate_i = i % population_size_;
        EncodedNAR mutant = MutateIndividual(population, candidate_i);
        NAR mutant_decoded = mutant.SetQualities(feature_domains, typed_relation_.get());
        double candidate_fitness = population[candidate_i].GetQualities().fitness;

        if (mutant.GetQualities().fitness > candidate_fitness) {
            population[candidate_i] = mutant;
            nar_collection_.emplace_back(mutant_decoded);
        }
    }

    auto compare_by_fitness = [](const NAR& a, const NAR& b) -> bool {
        return a.GetQualities().fitness > b.GetQualities().fitness;
    };
    std::sort(nar_collection_.begin(), nar_collection_.end(), compare_by_fitness);
    return 0;
}

}  // namespace algos::des
