#pragma once

#include <vector>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_tableau.h"

namespace algos::cfdfinder {
class PruningStrategy {
public:
    virtual ~PruningStrategy() = default;
    virtual void StartNewTableau(Candidate const& candidate) = 0;
    virtual void AddPattern(Pattern const& pattern) = 0;
    virtual void ExpandPattern(Pattern const& pattern) = 0;
    virtual void ProcessChild(Pattern& child) = 0;
    virtual bool HasEnoughPatterns(std::vector<Pattern> const& tableau) = 0;
    virtual bool IsPatternWorthConsidering(Pattern const& pattern) = 0;
    virtual bool IsPatternWorthAdding(Pattern const& pattern) = 0;
    virtual bool ValidForProcessing(Pattern const& child) = 0;
    virtual bool ContinueGeneration(PatternTableau const& currentTableau) = 0;
};
}  // namespace algos::cfdfinder
