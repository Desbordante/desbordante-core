#include "support_independent_strategy.h"

#include <boost/dynamic_bitset.hpp>

namespace {
std::vector<boost::dynamic_bitset<>> GenerateLhsSupersets(boost::dynamic_bitset<> const& lhs,
                                                          size_t num_attributes) {
    std::vector<boost::dynamic_bitset<>> supersets;
    for (size_t i = 0; i < num_attributes; ++i) {
        if (!lhs.test(i)) {
            boost::dynamic_bitset<> superset(lhs);
            superset.set(i);
            supersets.push_back(std::move(superset));
        }
    }
    return supersets;
}
}  // namespace

namespace algos::cfdfinder {

SupportIndependentPruning::SupportIndependentPruning(size_t pattern_threshold,
                                                     double min_support_gain,
                                                     double max_level_support_drop,
                                                     double min_confidence, size_t num_attributes)
    : max_patterns_(pattern_threshold),
      num_attributes_(num_attributes),
      min_support_gain_(min_support_gain),
      min_confidence_(min_confidence),
      max_level_support_drop_(max_level_support_drop),
      insufficient_support_gain_(false) {}

void SupportIndependentPruning::StartNewTableau(Candidate const& candidate) {
    insufficient_support_gain_ = false;
    current_candidate_ = candidate;
    visited_.clear();
}

void SupportIndependentPruning::AddPattern([[maybe_unused]] Pattern const& pattern) {
    visited_.clear();
}

void SupportIndependentPruning::ExpandPattern([[maybe_unused]] Pattern const& pattern) {}

void SupportIndependentPruning::ProcessChild(Pattern& child) {
    child.ClearCover();
    visited_.insert(child);
}

bool SupportIndependentPruning::HasEnoughPatterns(std::vector<Pattern> const& tableau) {
    return insufficient_support_gain_ || (tableau.size() >= max_patterns_);
}

bool SupportIndependentPruning::IsPatternWorthConsidering(Pattern const& pattern) {
    return pattern.GetSupport() >= min_support_gain_;
}

bool SupportIndependentPruning::IsPatternWorthAdding(Pattern const& pattern) {
    if (pattern.GetSupport() < min_support_gain_) {
        insufficient_support_gain_ = true;
        return false;
    }
    return pattern.GetConfidence() >= min_confidence_;
}

bool SupportIndependentPruning::ValidForProcessing(Pattern const& child) {
    return !visited_.contains(child);
}

bool SupportIndependentPruning::ContinueGeneration(PatternTableau const& current_tableau) {
    if (current_tableau.GetPatterns().empty() || current_tableau.GetSupport() == 0) {
        return false;
    }

    support_map_[current_candidate_] = current_tableau.GetSupport();
    double max_support = 0.0;

    for (auto&& superset : GenerateLhsSupersets(current_candidate_.lhs_, num_attributes_)) {
        Candidate parent(std::move(superset), current_candidate_.rhs_);

        if (support_map_.contains(parent)) {
            double support = support_map_.at(parent);
            max_support = std::max(max_support, support);
        }
    }

    return (max_support - max_level_support_drop_) <= current_tableau.GetSupport();
}

}  // namespace algos::cfdfinder