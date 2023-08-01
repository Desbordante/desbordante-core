#pragma once
#include "algorithms/fd/hycommon/sampler.h"
#include "algorithms/fd/hyfd/structures/non_fd_list.h"

namespace algos::hyfd {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records)
        : sampler_(std::move(plis), std::move(pli_records)) {}

    NonFDList GetNonFDs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::hyfd
