#pragma once

#include <unordered_map>
#include <vector>

#include "core/algorithms/cfd/cfun/quadruple.h"

namespace algos::cfd::cfun {
class AttributeIndex {
private:
    std::unordered_map<Quadruple::AttributeSet, Quadruple const*> index_;

public:
    AttributeIndex(std::vector<Quadruple> const& level);
    std::vector<Quadruple const*> GetAllSubsets(Quadruple const& attributes) const;
    bool AllSubsetsContains(Quadruple::AttributeSet attributes) const;
};
}  // namespace algos::cfd::cfun
