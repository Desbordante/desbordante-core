#include "core/algorithms/cfd/cfdfinder/model/expansion/positive_negative_constant_strategy.h"

#include <cstddef>
#include <utility>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/negative_constant_entry.h"

namespace algos::cfdfinder {

void PositiveNegativeConstantExpansion::Expand(
        Pattern&& parent_pattern, Frontier& frontier, Row const& inverted_pli_rhs,
        std::shared_ptr<PruningStrategy> pruning_strategy) const {
    auto entries_buffer = parent_pattern.GetEntries();
    auto const& cover = parent_pattern.GetCover();
    auto const const_entry_creator = [](int val) { return std::make_shared<ConstantEntry>(val); };
    auto const neg_entry_creator = [](int val) {
        return std::make_shared<NegativeConstantEntry>(val);
    };

    for (size_t replaced_index = 0; replaced_index < entries_buffer.size(); ++replaced_index) {
        auto const& item = entries_buffer[replaced_index];
        if (item.entry->IsConstantType()) {
            continue;
        }

        auto process_constants = [&](SupportMode support_mode, EntryCreator entry_creator) {
            boost::dynamic_bitset<> valid_constants = CalculateUniqueConstants(item.id, cover);

            auto cluster_representatives = GetClusterRepresentatives(item.id, cover);

            auto accumulated_support =
                    CalculateAccumulatedSupport(valid_constants, cluster_representatives, cover);

            FilterForSupport(valid_constants, pruning_strategy, accumulated_support, support_mode);

            if (valid_constants.empty()) {
                return;
            }

            FilterValidConstants(entries_buffer, replaced_index, valid_constants, pruning_strategy,
                                 entry_creator);
            if (valid_constants.empty()) {
                return;
            }

            auto cover_masks =
                    CalculateCoverMasks(std::move(valid_constants), cluster_representatives, cover);

            for (auto& [constant, cover_mask] : cover_masks) {
                if (support_mode == SupportMode::kComplement) {
                    cover_mask = ~cover_mask;
                }
                Entries new_entries = entries_buffer;
                new_entries[replaced_index].entry = entry_creator(constant);

                Pattern child(std::move(new_entries), BuildCover(cover, cover_mask),
                              inverted_pli_rhs);
                frontier.Emplace(std::move(child));
            }
        };

        process_constants(SupportMode::kDirect, const_entry_creator);
        process_constants(SupportMode::kComplement, neg_entry_creator);
    }
}

}  // namespace algos::cfdfinder
