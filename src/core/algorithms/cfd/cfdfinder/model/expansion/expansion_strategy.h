#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pruning/pruning_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/columns.h"
#include "core/algorithms/cfd/cfdfinder/types/frontier.h"

namespace algos::cfdfinder {

class ExpansionStrategy {
protected:
    using BitSet = boost::dynamic_bitset<>;
    using Cover = std::vector<Cluster>;

    ColumnRecords columns_values_;
    static Cover BuildCover(Cover const& cover, BitSet const& cover_mask);

    static bool IsValidReplacement(Entries& entries, size_t replaced_index,
                                   std::shared_ptr<Entry> replacement,
                                   PruningStrategy& pruning_strategy);

    std::vector<size_t> GetClusterRepresentatives(size_t id, Cover const& cover) const;

    virtual Entries GenerateNullEntries(BitSet const& lhs_attributes) const = 0;

public:
    explicit ExpansionStrategy(RowsPtr&& rows) : columns_values_(std::move(rows)) {}

    virtual ~ExpansionStrategy() = default;

    Pattern GenerateNullPattern(BitSet const& lhs_attributes, model::PLI const* lhs_pli,
                                Row const& inverted_pli_rhs) const;

    virtual void Expand(Pattern&& parent_pattern, Frontier& frontier, Row const& inverted_pli_rhs,
                        std::shared_ptr<PruningStrategy> pruning_strategy) const = 0;
};

}  // namespace algos::cfdfinder
