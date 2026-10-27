#pragma once

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <variant>
#include <vector>

#include <boost/unordered/unordered_flat_set.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/util/support_map.h"
#include "core/config/thread_number/type.h"

namespace algos::cfdfinder {

class SupportIndependentPruning : public PruningStrategy {
private:
    size_t max_patterns_;
    double min_support_gain_;
    double min_confidence_;
    double max_level_support_drop_;
    bool insufficient_support_gain_;
    utils::SupportMap support_map_;
    boost::unordered_flat_set<Entries, std::hash<Entries>> visited_;

protected:
    Candidate current_candidate_;

public:
    SupportIndependentPruning(size_t pattern_threshold, double min_support_gain,
                              double max_level_support_drop, double min_confidence,
                              config::ThreadNumType thread_num = 1)
        : max_patterns_(pattern_threshold),
          min_support_gain_(min_support_gain),
          min_confidence_(min_confidence),
          max_level_support_drop_(max_level_support_drop),
          insufficient_support_gain_(false),
          support_map_(thread_num) {}

    void StartNewTableau(Candidate const& candidate) override;
    bool HasEnoughPatterns(std::vector<Pattern> const& tableau) override;
    bool IsPatternWorthConsidering(double new_support) const override;
    bool TryAdding(Pattern& pattern) override;
    bool ValidForProcessing(Entries const& entries) override;
    bool ContinueGeneration(PatternTableau const& current_tableau) override;

    std::shared_ptr<PruningStrategy> Clone() const override {
        return std::make_shared<SupportIndependentPruning>(*this);
    }
};

}  // namespace algos::cfdfinder
