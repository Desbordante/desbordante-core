#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/ga_rfd/util/lru_cache.h"
#include "core/algorithms/rfd/rfd.h"
#include "core/config/custom_metric/custom_metrics/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_typed_relation_data.h"

namespace algos::rfd {

static constexpr std::size_t kMaxAttributes = 31;

class GaRfd final : public algos::Algorithm {
private:
    struct Individual {
        uint32_t lhs_mask = 0;
        uint8_t rhs_index = 0;
        double support = 0.0;
        double confidence = 0.0;

        bool operator==(Individual const& other) const {
            return lhs_mask == other.lhs_mask && rhs_index == other.rhs_index;
        }
    };

    struct IndividualHash {
        std::size_t operator()(Individual const& individual) const {
            return (static_cast<std::size_t>(individual.lhs_mask) << 8) | individual.rhs_index;
        }
    };

    // Input
    config::InputTable input_table_;
    config::CustomMetricsType metrics_;
    std::vector<std::string> column_names_;

    // Internal state
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    std::size_t num_attributes_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    std::vector<uint64_t> compute_buffer_;
    uint32_t full_mask_ = 0;

    // separate bin column on chunk of 64 bit
    std::vector<std::vector<uint64_t>> similar_pair_bits_;

    std::size_t cache_max_size_ = 10000;
    std::unique_ptr<util::LRUCache<uint32_t, std::size_t>> support_cache_;

    // Parameters
    std::vector<double> min_similarity_;  // similarity thresholds per attribute in [0, 1]
    double min_confidence_ = 1.0;         // minimum confidence for RFD
    std::size_t max_generations_ = 32;
    std::size_t max_population_size_ = 1024;
    double crossover_probability_ = 1.0;
    double mutation_probability_ = 1.0;
    std::uint32_t seed_ = 123;  // random number generator seed

    std::vector<RFD> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    void ExecuteInternal() final;
    void ResetState() final;

    // helper methods
    void BuildMatchBitsets();
    std::size_t ComputeSupport(uint32_t attributes_mask);
    // Computes conf and supp for a single individual
    Individual Evaluate(Individual const& individual);
    // Computes conf and supp for all individuals
    void EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& population);
    // Checks every individual's confidence meets the threshold
    bool AllConfidencesAboveThreshold(
            std::unordered_set<Individual, IndividualHash> const& population) const;
    // Computes fitness from confidence: 1.0 if confidence >= min_confidence_, else scaled.
    double Fitness(double confidence) const noexcept;

    // GA methods
    std::unordered_set<Individual, IndividualHash> InitializePopulation(
            std::mt19937& random_generator) const;
    std::unordered_set<Individual, IndividualHash> Select(
            std::unordered_set<Individual, IndividualHash> const& population,
            std::mt19937& random_generator) const;
    std::unordered_set<Individual, IndividualHash> Crossover(
            std::unordered_set<Individual, IndividualHash> const& selected,
            std::mt19937& random_generator) const;
    std::unordered_set<Individual, IndividualHash> Mutate(
            std::unordered_set<Individual, IndividualHash> const& population,
            std::mt19937& random_generator) const;

    std::vector<RFD> Finalize(
            std::unordered_set<Individual, IndividualHash> const& population) const;

public:
    GaRfd();

    [[nodiscard]] std::vector<RFD> GetRfds() const {
        std::vector<RFD> rfds = discovered_;
        std::sort(rfds.begin(), rfds.end());
        return rfds;
    }
};

}  // namespace algos::rfd
