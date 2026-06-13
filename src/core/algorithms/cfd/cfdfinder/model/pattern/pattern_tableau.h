#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/model/pattern/entry.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/variable_entry.h"

namespace algos::cfdfinder {
class PatternTableau {
private:
    std::vector<Pattern> patterns_;
    size_t num_tuples_;

    size_t GetGlobalCover() const;
    size_t GetGlobalKeepers() const;

public:
    PatternTableau(std::vector<Pattern> patterns, size_t num_tuples)
        : patterns_(std::move(patterns)), num_tuples_(num_tuples) {}

    std::vector<Pattern> const& GetPatterns() const {
        return patterns_;
    };

    double GetSupport() const;
    double GetConfidence() const;
};
}  // namespace algos::cfdfinder
