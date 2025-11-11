#pragma once

#include "algorithms/fd/hycommon/types.h"
#include "expansion_strategy.h"

namespace algos::cfdfinder {

class ConstantExpansion : public ExpansionStrategy {
protected:
    hy::RowsPtr compressed_records_;

    virtual std::list<Pattern> GetChildPatterns(Pattern const& pattern, Cluster const& cluster);

public:
    explicit ConstantExpansion(hy::RowsPtr compressed_records)
        : compressed_records_(std::move(compressed_records)) {}

    Pattern GenerateNullPattern(BitSet const& attributes) override;
    std::list<Pattern> GetChildPatterns(Pattern const& current_pattern) override;
};

}  // namespace algos::cfdfinder