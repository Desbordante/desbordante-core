#pragma once

#include <cstddef>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {

class PartialFdPruning : public PruningStrategy {
private:
    size_t num_rows_;
    double max_g1_;
    ColumnsPtr const inverted_plis_;
    Row inverted_pli_rhs_;

    unsigned long long CalculateViolations(Pattern const& pattern) const;

    double CalculateG1(Pattern const& pattern) const;

public:
    PartialFdPruning(size_t num_rows, double max_g1, ColumnsPtr&& inverted_plis)
        : num_rows_(num_rows), max_g1_(max_g1), inverted_plis_(std::move(inverted_plis)) {}

    void StartNewTableau(Candidate const& candidate) override {
        inverted_pli_rhs_ = inverted_plis_->at(candidate.rhs_);
    }

    bool HasEnoughPatterns(std::vector<Pattern> const& tableau) override {
        return !tableau.empty();
    }

    bool IsPatternWorthConsidering([[maybe_unused]] double new_support) const override {
        return false;
    }

    bool TryAdding(Pattern& pattern) override {
        return CalculateG1(pattern) <= max_g1_;
    }

    bool ValidForProcessing([[maybe_unused]] Entries const& entries) override {
        return false;
    }

    bool ContinueGeneration(PatternTableau const& current_tableau) override {
        return current_tableau.GetPatterns().size() == 1;
    }

    std::shared_ptr<PruningStrategy> Clone() const override {
        return std::make_shared<PartialFdPruning>(*this);
    }
};

}  // namespace algos::cfdfinder
