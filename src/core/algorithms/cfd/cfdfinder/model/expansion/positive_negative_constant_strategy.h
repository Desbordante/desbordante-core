#pragma once

#include "constant_expansion_strategy.h"

namespace algos::cfdfinder {
class PositiveNegativeConstantExpansion : public ConstantExpansion {
private:
    std::list<Pattern> GetChildPatterns(Pattern const& pattern, Cluster const& cluster) override;

public:
    PositiveNegativeConstantExpansion(hy::RowsPtr compressed_records)
        : ConstantExpansion(std::move(compressed_records)) {}
};

}  // namespace algos::cfdfinder