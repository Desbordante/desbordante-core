#pragma once

#include <cstddef>
#include <unordered_set>

#include "algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"

namespace algos::cfdfinder {

class LegacyPruning : public PruningStrategy {
private:
    double min_support_;
    double min_confidence_;
    double cumulative_support_;
    size_t num_tuples_;
    std::unordered_set<Pattern> visited_;

public:
    LegacyPruning(double min_support, double min_confidence, size_t num_tuples)
        : min_support_(min_support),
          min_confidence_(min_confidence),
          cumulative_support_(0),
          num_tuples_(num_tuples) {}

    void StartNewTableau([[maybe_unused]] Candidate const& candidate) override;

    void AddPattern(Pattern const& pattern) override;

    void ExpandPattern(Pattern const& pattern) override;

    void ProcessChild([[maybe_unused]] Pattern& child) override {}

    bool HasEnoughPatterns([[maybe_unused]] std::vector<Pattern> const& tableau) override;

    bool IsPatternWorthConsidering(Pattern const& pattern) override;

    bool IsPatternWorthAdding(Pattern const& pattern) override;

    bool ValidForProcessing(Pattern const& child) override;

    bool ContinueGeneration(PatternTableau const& currentTableau) override;
};
}  // namespace algos::cfdfinder
