#pragma once

#include <memory>
#include <vector>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_tableau.h"

namespace algos::cfdfinder {
class PruningStrategy {
public:
    virtual ~PruningStrategy() = default;
    virtual void StartNewTableau(Candidate const& candidate) = 0;
    virtual bool HasEnoughPatterns(std::vector<Pattern> const& tableau) = 0;
    virtual bool IsPatternWorthConsidering(double new_support) const = 0;
    virtual bool TryAdding(Pattern& pattern) = 0;
    virtual bool ValidForProcessing(Entries const& entries) = 0;
    virtual bool ContinueGeneration(PatternTableau const& currentTableau) = 0;
    virtual std::shared_ptr<PruningStrategy> Clone() const = 0;
};
}  // namespace algos::cfdfinder
