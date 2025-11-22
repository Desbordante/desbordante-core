#pragma once
#include <list>

#include "algorithms/cfd/cfdfinder/model/expansion/constant_expansion_strategy.h"
#include "algorithms/cfd/cfdfinder/types/hyfd_types.h"

namespace algos::cfdfinder {
class PositiveNegativeConstantExpansion : public ConstantExpansion {
private:
    std::list<Pattern> GetChildPatterns(Pattern const& pattern,
                                        Cluster const& cluster) const override;

public:
    explicit PositiveNegativeConstantExpansion(RowsPtr&& compressed_records)
        : ConstantExpansion(std::move(compressed_records)) {}
};

}  // namespace algos::cfdfinder
