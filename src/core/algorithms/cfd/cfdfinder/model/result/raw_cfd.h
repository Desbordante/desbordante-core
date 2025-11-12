#pragma once

#include "cfd/cfdfinder/candidate.h"
#include "cfd/cfdfinder/model/pattern/pattern_tableau.h"

namespace algos::cfdfinder {
struct RawCFD {
    Candidate embedded_fd_;
    PatternTableau patterns_;
};

}  // namespace algos::cfdfinder