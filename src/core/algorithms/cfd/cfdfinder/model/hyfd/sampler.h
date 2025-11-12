#pragma once

#include "fd/hyfd/sampler.h"

namespace algos::cfdfinder {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, config::ThreadNumType threads_num = 1)
        : sampler_(std::move(plis), std::move(pli_records), threads_num) {}

    hyfd::NonFDList GetNonFDs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::cfdfinder
