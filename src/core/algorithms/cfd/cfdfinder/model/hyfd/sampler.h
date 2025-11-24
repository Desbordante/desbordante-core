#pragma once

#include "algorithms/cfd/cfdfinder/types/hyfd_types.h"
#include "algorithms/fd/hyfd/sampler.h"
#include "config/thread_number/type.h"

namespace algos::cfdfinder {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(PLIsPtr plis, RowsPtr pli_records, config::ThreadNumType threads_num = 1)
        : sampler_(std::move(plis), std::move(pli_records), threads_num) {}

    hyfd::NonFDList GetNonFDs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::cfdfinder
