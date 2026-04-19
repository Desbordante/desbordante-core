#pragma once

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/rfd/similarity_metric.h"
#include "core/config/tabular_data/input_table_type.h"

namespace algos::rfd {

class GaRfd : public Algorithm {
private:
    // Input
    config::InputTable table_;
    std::vector<std::shared_ptr<SimilarityMetric>> metrics_;

    // Parameters
    std::vector<double> similarity_thresholds_;
    double min_confidence_ = 0.9;
    size_t population_size_ = 100;
    size_t max_generations_ = 500;
    double crossover_prob_ = 0.85;
    double mutation_prob_ = 0.3;
    size_t max_lhs_arity_ = 3;
    uint64_t seed_ = 42;
    std::string output_file_;

    // Internal state
    std::vector<std::vector<std::string>> data_;
    size_t num_attrs_ = 0;
    size_t num_rows_ = 0;
    size_t total_pairs_ = 0;
    std::vector<boost::dynamic_bitset<>> attr_bitsets_;
    mutable std::unordered_map<uint64_t, size_t> support_cache_;

    std::mt19937 rng_;

    struct Individual {
        std::vector<size_t> genes;
        double fitness = 0.0;
    };
    std::vector<Individual> discovered_rfds_;

    // Algorithm overrides
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() override;
    void ResetState() override;
    void MakeExecuteOptsAvailable() override;

    // GA helper methods
    void BuildSimilarityBitsets();
    size_t ComputeSupport(uint64_t attrs_mask) const;
    double CalculateConfidence(uint64_t lhs_mask, size_t rhs);
    void EvaluatePopulation(std::vector<Individual>& population);
    std::vector<Individual> InitializePopulation();
    std::vector<Individual> Selection(const std::vector<Individual>& population);
    std::vector<Individual> Crossover(const std::vector<Individual>& parents);
    std::vector<Individual> Mutation(std::vector<Individual> individuals);
    void RegisterResults(const std::vector<Individual>& final_population);

public:
    GaRfd();

    // Setters for Python bindings
    void SetSimilarityThresholds(const std::vector<double>& val) { similarity_thresholds_ = val; }
    void SetMinConfidence(double val) { min_confidence_ = val; }
    void SetPopulationSize(size_t val) { population_size_ = val; }
    void SetMaxGenerations(size_t val) { max_generations_ = val; }
    void SetCrossoverProb(double val) { crossover_prob_ = val; }
    void SetMutationProb(double val) { mutation_prob_ = val; }
    void SetMaxLhsArity(size_t val) { max_lhs_arity_ = val; }
    void SetSeed(uint64_t val) { seed_ = val; rng_.seed(val); }
    void SetOutputFile(const std::string& val) { output_file_ = val; }
    void SetMetrics(std::vector<std::shared_ptr<SimilarityMetric>> metrics) { metrics_ = std::move(metrics); }

    std::vector<std::string> GetResultStrings() const;
    void SaveResults(const std::string& filepath) const;
};

}  // namespace algos::rfd
