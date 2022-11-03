#pragma once

#include "structures/non_fd_list.h"
#include "types.h"

namespace algos::hyfd {

class Sampler {
public:
    Sampler(PLIs const& plis, RowsPtr pli_records);
    ~Sampler();

    NonFDList GetNonFDCandidates(IdPairs const& comparison_suggestions);
};

}  // namespace algos::hyfd
