#include "core/algorithms/cfd/cfdfinder/model/expansion/constant_expansion_strategy.h"

#include <cstddef>
#include <memory>
#include <utility>

#include "core/algorithms/cfd/cfdfinder/model/pattern/constant_entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/variable_entry.h"
#include "core/util/bitset_utils.h"

namespace algos::cfdfinder {

Entries ConstantExpansion::GenerateNullEntries(BitSet const& attributes) const {
    Entries null_entries;
    util::ForEachIndex(attributes, [&](size_t attr) {
        null_entries.emplace_back(attr, std::make_shared<VariableEntry>());
    });

    return null_entries;
}

boost::dynamic_bitset<> ConstantExpansion::CalculateUniqueConstants(size_t column_id,
                                                                    Cover const& cover) const {
    boost::dynamic_bitset<> unique_constants(columns_values_.GetMaxValue(column_id));
    auto const& column = columns_values_.GetColumn(column_id);

    for (auto const& cluster : cover) {
        unique_constants.set(column[cluster[0]]);
    }

    return unique_constants;
}

ConstantExpansion::SupportByConstant ConstantExpansion::CalculateAccumulatedSupport(
        boost::dynamic_bitset<> const& constants,
        std::vector<size_t> const& cluster_representatives, Cover const& cover) const {
    SupportByConstant accumulated_support;
    accumulated_support.reserve(constants.count());

    util::ForEachIndex(constants, [&](size_t constant) { accumulated_support[constant] = 0; });

    for (size_t cluster_id = 0; cluster_id < cover.size(); ++cluster_id) {
        int constant = cluster_representatives[cluster_id];
        if (auto it = accumulated_support.find(constant); it != accumulated_support.end()) {
            it->second += cover[cluster_id].size();
        }
    }

    return accumulated_support;
}

void ConstantExpansion::FilterForSupport(boost::dynamic_bitset<>& constants,
                                         std::shared_ptr<PruningStrategy> pruning_strategy,
                                         SupportByConstant const& accumulated_support,
                                         SupportMode support_mode) const {
    auto const num_rows = columns_values_.GetColumn(0).size();

    for (auto constant = constants.find_first(); constant != boost::dynamic_bitset<>::npos;
         constant = constants.find_next(constant)) {
        size_t support = support_mode == SupportMode::kDirect
                                 ? accumulated_support.at(constant)
                                 : num_rows - accumulated_support.at(constant);
        if (!pruning_strategy->IsPatternWorthConsidering(support)) {
            constants.reset(constant);
        }
    }
}

std::vector<ConstantExpansion::CoverMask> ConstantExpansion::CalculateCoverMasks(
        boost::dynamic_bitset<>&& valid_constants,
        std::vector<size_t> const& cluster_representatives, Cover const& cover) const {
    boost::unordered_flat_map<int, size_t> constant_to_idx;
    constant_to_idx.reserve(valid_constants.count());

    std::vector<CoverMask> cover_masks;
    cover_masks.reserve(valid_constants.count());

    util::ForEachIndex(valid_constants, [&](size_t val) {
        constant_to_idx.emplace(val, cover_masks.size());
        cover_masks.emplace_back(static_cast<int>(val), boost::dynamic_bitset<>(cover.size()));
    });

    for (size_t cluster_id = 0; cluster_id < cover.size(); ++cluster_id) {
        int val = cluster_representatives[cluster_id];

        if (auto it = constant_to_idx.find(val); it != constant_to_idx.end()) {
            size_t idx = it->second;
            cover_masks[idx].second.set(cluster_id);
        }
    }
    return cover_masks;
}

void ConstantExpansion::FilterValidConstants(Entries& entries_buffer, size_t replaced_index,
                                             boost::dynamic_bitset<>& constants,
                                             std::shared_ptr<PruningStrategy> pruning_strategy,
                                             EntryCreator const& create_entry) const {
    for (auto constant = constants.find_first(); constant != boost::dynamic_bitset<>::npos;
         constant = constants.find_next(constant)) {
        if (!IsValidReplacement(entries_buffer, replaced_index, create_entry(constant),
                                *pruning_strategy)) {
            constants.reset(constant);
        }
    }
}

void ConstantExpansion::Expand(Pattern&& parent_pattern, Frontier& frontier,
                               Row const& inverted_pli_rhs,
                               std::shared_ptr<PruningStrategy> pruning_strategy) const {
    auto entries_buffer = parent_pattern.GetEntries();
    auto const& cover = parent_pattern.GetCover();
    auto const const_entry_creator = [](int val) { return std::make_shared<ConstantEntry>(val); };

    for (size_t replaced_index = 0; replaced_index < entries_buffer.size(); ++replaced_index) {
        auto const& item = entries_buffer[replaced_index];
        if (item.entry->IsConstantType()) {
            continue;
        }

        boost::dynamic_bitset<> valid_constants = CalculateUniqueConstants(item.id, cover);

        auto cluster_representatives = GetClusterRepresentatives(item.id, cover);

        auto accumulated_support =
                CalculateAccumulatedSupport(valid_constants, cluster_representatives, cover);

        FilterForSupport(valid_constants, pruning_strategy, accumulated_support,
                         SupportMode::kDirect);

        if (!valid_constants.any()) {
            continue;
        }

        FilterValidConstants(entries_buffer, replaced_index, valid_constants, pruning_strategy,
                             const_entry_creator);

        if (!valid_constants.any()) {
            continue;
        }

        auto cover_masks =
                CalculateCoverMasks(std::move(valid_constants), cluster_representatives, cover);

        for (auto& [constant, cover_mask] : cover_masks) {
            Entries new_entries = entries_buffer;
            new_entries[replaced_index].entry = const_entry_creator(constant);

            Pattern child(std::move(new_entries), BuildCover(cover, cover_mask), inverted_pli_rhs);
            frontier.Emplace(std::move(child));
        }
    }
}

}  // namespace algos::cfdfinder
