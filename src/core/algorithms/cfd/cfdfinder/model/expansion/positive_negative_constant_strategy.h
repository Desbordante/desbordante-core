#pragma once

#include "core/algorithms/cfd/cfdfinder/model/expansion/constant_expansion_strategy.h"
#include "core/algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {
class PositiveNegativeConstantExpansion : public ConstantExpansion {
public:
    explicit PositiveNegativeConstantExpansion(RowsPtr&& compressed_records)
        : ConstantExpansion(std::move(compressed_records)) {}

    void Expand(Pattern&& parent_pattern, Frontier& frontier, Row const& inverted_pli_rhs,
                std::shared_ptr<PruningStrategy> pruning_strategy) const override;
};

}  // namespace algos::cfdfinder
