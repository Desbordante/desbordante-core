#pragma once

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
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"

namespace tests {
class GaRfdTester;
}

namespace algos::rfd {

class GaRfd final : public algos::Algorithm {
public:
    struct Individual {
        uint32_t lhs_mask = 0;
        uint8_t rhs_index = 0;
        double confidence = 0.0;
        double support = 0.0;

        bool operator<(Individual const& other) const {
            if (lhs_mask != other.lhs_mask) return lhs_mask < other.lhs_mask;
            return rhs_index < other.rhs_index;
        }

        bool operator==(Individual const& other) const {
            return lhs_mask == other.lhs_mask && rhs_index == other.rhs_index;
        }
    };

    struct IndividualHash {
        std::size_t operator()(Individual const& ind) const {
            return (static_cast<std::size_t>(ind.lhs_mask) << 8) | ind.rhs_index;
        }
    };

private:
    // Input
    config::InputTable input_table_;
    std::vector<std::shared_ptr<SimilarityMetric>> metrics_;

    // Internal state
    std::vector<std::vector<std::string>> column_data_;
    uint8_t num_attrs_ = 0;
    std::size_t num_rows_ = 0;
    std::size_t total_pairs_ = 0;

    mutable std::vector<uint64_t> compute_buffer_;
    uint32_t full_mask_;

    // separate bin column on chunk of 64 bit
    std::vector<std::vector<uint64_t>> attr_similarity_bits_;

    std::size_t cache_max_size_ = 10000;
    mutable std::unique_ptr<util::LRUCache<uint32_t, std::size_t>> support_cache_;

    // Parameters
    std::vector<double> min_similarity_;  // similarity thresholds per attribute
    double eps_ = 1.0;                    // minimum confidence for RFD
    std::size_t max_generations_ = 32;
    std::size_t population_size_ = 1024;
    double crossover_probability_ = 1.0;
    double mutation_probability_ = 1.0;
    std::uint32_t seed_ = std::random_device{}();  // random number generator seed

    std::unordered_set<RFD, RFDHash> discovered_;

    // Algorithm overrides
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void LoadDataInternal() final;
    unsigned long long ExecuteInternal() final;
    void ResetState() final;

    // helper methods
    void BuildSimilarityBitsets();
    [[nodiscard]] std::size_t ComputeSupport(uint32_t attrs_mask) const;
    // Computes conf and supp for a single individual
    [[nodiscard]] Individual Evaluate(Individual const& ind) const;
    // Computes conf and supp for all individuals
    void EvaluatePopulation(std::unordered_set<Individual, IndividualHash>& pop) const;
    // Checks each individual threshold satisfies conf
    [[nodiscard]] bool AllOf(std::unordered_set<Individual, IndividualHash> const& pop) const;
    // Computes fitness from conf: 1.0 if confidence >= beta, else confidence / beta.
    [[nodiscard]] double Fitness(double confidence) const noexcept;

    // GA methods
    [[nodiscard]] std::unordered_set<Individual, IndividualHash> InitializePopulation(
            std::mt19937& rng) const;
    [[nodiscard]] std::unordered_set<Individual, IndividualHash> Select(
            std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const;
    [[nodiscard]] std::unordered_set<Individual, IndividualHash> Crossover(
            std::unordered_set<Individual, IndividualHash> const& selected,
            std::mt19937& rng) const;
    [[nodiscard]] std::unordered_set<Individual, IndividualHash> Mutate(
            std::unordered_set<Individual, IndividualHash> const& pop, std::mt19937& rng) const;

    [[nodiscard]] std::unordered_set<RFD, RFDHash> Finalize(
            std::unordered_set<Individual, IndividualHash> const& pop) const;

    friend class tests::GaRfdTester;

public:
    GaRfd();
    ~GaRfd() override;

    void SetMetrics(std::vector<std::shared_ptr<SimilarityMetric>> metrics) noexcept {
        metrics_ = std::move(metrics);
    }

    [[nodiscard]] std::vector<RFD> GetRfds() const {
        return {discovered_.begin(), discovered_.end()};
    }
};

}  // namespace algos::rfd
