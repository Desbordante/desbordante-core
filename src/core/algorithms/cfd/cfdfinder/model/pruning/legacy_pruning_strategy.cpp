#include "legacy_pruning_strategy.h"

#include <fstream>

#include "cfd/cfdfinder/model/pattern/variable_entry.h"

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
    return cumulative_support_ >= min_support_ * static_cast<double>(num_tuples_);
}

bool LegacyPruning::IsPatternWorthConsidering(Pattern const& pattern) {
    return pattern.GetSupport() > 0;
}

bool LegacyPruning::IsPatternWorthAdding(Pattern const& pattern) {
    return pattern.GetConfidence() >= min_confidence_;
}

bool LegacyPruning::ValidForProcessing(Pattern const& child) {
    if (PatternDebugController::IsDebugEnabled()) {
        std::list<Pattern> parent_patterns;
        auto const& entries = child.GetEntries();
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].entry->GetType() == EntryType::kConstant ||
                entries[i].entry->GetType() == EntryType::kNegativeConstant) {
                auto new_entries = entries;
                new_entries[i].entry = std::make_shared<VariableEntry>();
                parent_patterns.emplace_back(std::move(new_entries));
            }
        }
        for (auto const& parent : parent_patterns) {
            if (!visited_.contains(parent)) {
                return false;
            }
        }
        return true;
    } else {
        auto const& entries = child.GetEntries();
        for (size_t i = 0; i < entries.size(); ++i) {
            if (entries[i].entry->GetType() == EntryType::kConstant ||
                entries[i].entry->GetType() == EntryType::kNegativeConstant) {
                auto new_entries = entries;
                new_entries[i].entry = std::make_shared<VariableEntry>();
                if (!visited_.contains(Pattern(std::move(new_entries)))) {
                    return false;
                }
            }
        }
        return true;
    }
}

bool LegacyPruning::ContinueGeneration(PatternTableau const& currentTableau) {
    return currentTableau.GetSupport() >= min_support_ &&
           currentTableau.GetConfidence() >= min_confidence_;
}

}  // namespace algos::cfdfinder