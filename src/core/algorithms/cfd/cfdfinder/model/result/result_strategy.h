#pragma once
#include <list>

#include "core/algorithms/cfd/cfdfinder/candidate.h"
#include "core/algorithms/cfd/cfdfinder/model/pattern/pattern_tableau.h"
#include "core/algorithms/cfd/cfdfinder/model/result/raw_cfd.h"

namespace algos::cfdfinder {
class ResultStrategy {
public:
    virtual ~ResultStrategy() = default;
    virtual void ReceiveResult(Candidate embedded_fd, PatternTableau tableau) = 0;
    virtual std::list<RawCFD> TakeAllResults() = 0;
};
}  // namespace algos::cfdfinder
