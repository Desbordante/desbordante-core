#pragma once

#include <memory>
#include <vector>

#include "expansion_strategy.h"

namespace algos::cfdfinder {

class ConstantExpansion : public ExpansionStrategy {
protected:
    hy::RowsPtr compressed_records_;

    virtual std::list<Pattern> GetChildPatterns(Pattern const& pattern, Cluster const& cluster);

public:
    explicit ConstantExpansion(hy::RowsPtr compressed_records)
        : compressed_records_(std::move(compressed_records)) {}

    Pattern GenerateNullPattern(boost::dynamic_bitset<> const& attributes) override;
    std::list<Pattern> GetChildPatterns(Pattern const& current_pattern) override;
};

}  // namespace algos::cfdfinder