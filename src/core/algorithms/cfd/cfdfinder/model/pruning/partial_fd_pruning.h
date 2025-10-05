#pragma once

#include <cmath>
#include <unordered_map>

#include "fd/hycommon/util/pli_util.h"
#include "pruning_strategy.h"

namespace algos::cfdfinder {

class PartialFdPruning : public PruningStrategy {
private:
    size_t num_records_;
    double max_g1_;
    hy::RowsPtr const inverted_plis_;
    hy::Row inverted_pli_rhs_;

    unsigned long long CalculateViolations(Pattern const& pattern) const {
        unsigned long long violations = 0;

        for (auto const& cluster : pattern.GetCover()) {
            size_t cluster_size = cluster.size();
            std::unordered_map<int, size_t> value_counts;

            for (auto index : cluster) {
                auto value = inverted_pli_rhs_[index];
                if (!algos::hy::PLIUtil::IsSingletonCluster(value)) {
                    value_counts[value]++;
                }
            }

            size_t total_pairs = cluster_size * cluster_size;

            size_t non_violation_pairs = 0;
            for (auto const& entry : value_counts) {
                non_violation_pairs += entry.second * entry.second;
            }

            size_t cluster_violations = total_pairs - non_violation_pairs;
            violations += cluster_violations;
        }

        return violations;
    }

    double CalculateG1(Pattern const& pattern) const {
        unsigned long long violations = CalculateViolations(pattern);
        double normalization = std::pow(num_records_, 2) - num_records_;
        return static_cast<double>(violations) / normalization;
    }

public:
    PartialFdPruning(size_t num_records, double max_g1, hy::RowsPtr inverted_plis)
        : num_records_(num_records), max_g1_(max_g1), inverted_plis_(std::move(inverted_plis)) {}

    void StartNewTableau(Candidate const& candidate) override {
        inverted_pli_rhs_ = inverted_plis_->at(candidate.rhs_);
    }

    void AddPattern([[maybe_unused]] Pattern const& pattern) override {}

    void ExpandPattern([[maybe_unused]] Pattern const& pattern) override {}

    void ProcessChild([[maybe_unused]] Pattern& child) override {}

    bool HasEnoughPatterns(std::vector<Pattern> const& tableau) override {
        return !tableau.empty();
    }

    bool IsPatternWorthConsidering([[maybe_unused]] Pattern const& pattern) override {
        return false;
    }

    bool IsPatternWorthAdding(Pattern const& pattern) override {
        return CalculateG1(pattern) <= max_g1_;
    }

    bool ValidForProcessing([[maybe_unused]] Pattern const& child) override {
        return false;
    }

    bool ContinueGeneration(PatternTableau const& current_tableau) override {
        return current_tableau.GetPatterns().size() == 1;
    }
};

}  // namespace algos::cfdfinder