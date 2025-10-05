#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pruning_strategy.h"

namespace algos::cfdfinder {

class SupportIndependentPruning : public PruningStrategy {
private:
    size_t max_patterns_;
    size_t num_attributes_;
    double min_support_gain_;
    double min_confidence_;
    double max_level_support_drop_;
    bool insufficient_support_gain_;
    std::unordered_map<Candidate, double> support_map_;
    std::unordered_set<Pattern> visited_;

protected:
    Candidate current_candidate_;

public:
    SupportIndependentPruning(size_t pattern_threshold, double min_support_gain,
                              double max_level_support_drop, double min_confidence,
                              size_t num_attributes);

    void StartNewTableau(Candidate const& candidate) override;
    void AddPattern([[maybe_unused]] Pattern const& pattern) override;
    void ExpandPattern([[maybe_unused]] Pattern const& pattern) override;
    void ProcessChild(Pattern& child) override;
    bool HasEnoughPatterns(std::vector<Pattern> const& tableau) override;
    bool IsPatternWorthConsidering(Pattern const& pattern) override;
    bool IsPatternWorthAdding(Pattern const& pattern) override;
    bool ValidForProcessing(Pattern const& child) override;
    bool ContinueGeneration(PatternTableau const& current_tableau) override;
};

}  // namespace algos::cfdfinder