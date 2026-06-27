#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include "core/algorithms/cfd/cfdfinder/model/expansion/expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {

class ConstantExpansion : public ExpansionStrategy {
protected:
    enum class SupportMode : char { kDirect, kComplement };
    using EntryCreator = std::function<std::shared_ptr<Entry>(int)>;
    using CoverMask = std::pair<int, boost::dynamic_bitset<>>;
    using SupportByConstant = boost::unordered_flat_map<int, size_t>;

    boost::dynamic_bitset<> CalculateUniqueConstants(size_t column_id, Cover const& cover) const;
    SupportByConstant CalculateAccumulatedSupport(
            boost::dynamic_bitset<> const& constants,
            std::vector<size_t> const& cluster_representatives, Cover const& cover) const;
    void FilterForSupport(boost::dynamic_bitset<>& valid_constants,
                          std::shared_ptr<PruningStrategy> pruning_strategy,
                          SupportByConstant const& accumulated_support,
                          SupportMode support_mode) const;

    std::vector<CoverMask> CalculateCoverMasks(boost::dynamic_bitset<>&& processed_values,
                                               std::vector<size_t> const& cluster_representatives,
                                               Cover const& cover) const;

    Entries GenerateNullEntries(BitSet const& attributes) const override;

    void FilterValidConstants(Entries& entries_buffer, size_t replaced_index,
                              boost::dynamic_bitset<>& constants,
                              std::shared_ptr<PruningStrategy> pruning_strategy,
                              EntryCreator const& create_entry) const;

public:
    explicit ConstantExpansion(RowsPtr&& compressed_records)
        : ExpansionStrategy(std::move(compressed_records)) {}

    void Expand(Pattern&& parent_pattern, Frontier& frontier, Row const& inverted_pli_rhs,
                std::shared_ptr<PruningStrategy> pruning_strategy) const override;
};

}  // namespace algos::cfdfinder
