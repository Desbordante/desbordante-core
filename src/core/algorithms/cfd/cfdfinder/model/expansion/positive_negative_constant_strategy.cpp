#include "positive_negative_constant_strategy.h"

#include "algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "algorithms/cfd/cfdfinder/model/pattern/negative_constant_entry.h"

namespace algos::cfdfinder {

std::list<Pattern> PositiveNegativeConstantExpansion::GetChildPatterns(Pattern const& pattern,
                                                                       Cluster const& cluster) {
    std::list<Pattern> results;
    auto const& entries = pattern.GetEntries();
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].entry->GetType() != EntryType::kVariable) {
            continue;
        }
        int value = compressed_records_->at(cluster[0])[entries[i].id];

        auto new_entries = entries;
        new_entries[i].entry = std::make_shared<ConstantEntry>(value);
        results.emplace_back(std::move(new_entries));

        new_entries = entries;
        new_entries[i].entry = std::make_shared<NegativeConstantEntry>(value);
        results.emplace_back(std::move(new_entries));
    }

    return results;
}

}  // namespace algos::cfdfinder