#include "core/algorithms/cfd/cfdfinder/model/pruning/legacy_pruning_strategy.h"

#include <list>
#include <memory>
#include <ranges>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_tableau.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/variable_entry.h"

namespace algos::cfdfinder {

void LegacyPruning::StartNewTableau([[maybe_unused]] Candidate const& candidate) {
    cumulative_support_ = 0;
    visited_.clear();
}

void LegacyPruning::AddPattern(Pattern const& pattern) {
    cumulative_support_ += pattern.GetSupport();
}

void LegacyPruning::ExpandPattern(Pattern const& pattern) {
    visited_.insert(pattern);
}

bool LegacyPruning::HasEnoughPatterns([[maybe_unused]] std::vector<Pattern> const& tableau) {
    return cumulative_support_ >= min_support_ * num_tuples_;
}

bool LegacyPruning::IsPatternWorthConsidering(Pattern const& pattern) {
    return pattern.GetSupport() > 0;
}

bool LegacyPruning::IsPatternWorthAdding(Pattern const& pattern) {
    return pattern.GetConfidence() >= min_confidence_;
}

bool LegacyPruning::ValidForProcessing(Pattern const& child) {
    std::list<Pattern> parent_patterns;
    auto const& entries = child.GetEntries();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].entry->IsConstant()) {
            auto new_entries = entries;
            new_entries[i].entry = std::make_shared<VariableEntry>();
            parent_patterns.emplace_back(std::move(new_entries));
        }
    }

    return std::ranges::all_of(parent_patterns,
                               [this](auto const& parent) { return visited_.contains(parent); });
}

bool LegacyPruning::ContinueGeneration(PatternTableau const& currentTableau) {
    return currentTableau.GetSupport() >= min_support_ &&
           currentTableau.GetConfidence() >= min_confidence_;
}

}  // namespace algos::cfdfinder
