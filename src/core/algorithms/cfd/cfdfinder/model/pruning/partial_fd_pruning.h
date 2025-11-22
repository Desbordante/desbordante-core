#pragma once

#include <cstddef>

#include "algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"

namespace algos::cfdfinder {

class PartialFdPruning : public PruningStrategy {
private:
    size_t num_records_;
    double max_g1_;
    ColumnsPtr const inverted_plis_;
    Row inverted_pli_rhs_;

    unsigned long long CalculateViolations(Pattern const& pattern) const;

    double CalculateG1(Pattern const& pattern) const;

public:
    PartialFdPruning(size_t num_records, double max_g1, ColumnsPtr&& inverted_plis)
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
