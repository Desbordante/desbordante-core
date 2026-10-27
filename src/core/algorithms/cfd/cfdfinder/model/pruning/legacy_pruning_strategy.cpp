#include "core/algorithms/cfd/cfdfinder/model/pruning/legacy_pruning_strategy.h"

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

bool LegacyPruning::HasEnoughPatterns([[maybe_unused]] std::vector<Pattern> const& tableau) {
    return cumulative_support_ >= min_support_ * num_rows_;
}

bool LegacyPruning::IsPatternWorthConsidering(double new_support) const {
    return new_support > 0;
}

bool LegacyPruning::TryAdding(Pattern& pattern) {
    if (pattern.GetConfidence() >= min_confidence_) {
        cumulative_support_ += pattern.GetSupport();
        return true;
    }

    visited_.insert(pattern.GetEntries());
    return false;
}

bool LegacyPruning::ValidForProcessing(Entries const& entries) {
    auto buffer_entries = entries;
    std::shared_ptr<Entry> buff_entry = std::make_shared<VariableEntry>();

    auto is_visited = [&](std::shared_ptr<Entry> entry) -> bool {
        std::swap(entry, buff_entry);
        bool is_visit = visited_.contains(buffer_entries);
        std::swap(entry, buff_entry);
        return is_visit;
    };

    for (auto& [_, entry] : buffer_entries) {
        if (entry->IsConstantType() && is_visited(entry)) {
            return false;
        }
    }
    return true;
}

bool LegacyPruning::ContinueGeneration(PatternTableau const& currentTableau) {
    return currentTableau.GetSupport() >= min_support_ &&
           currentTableau.GetConfidence() >= min_confidence_;
}

}  // namespace algos::cfdfinder
