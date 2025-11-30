#pragma once

#include "core/algorithms/fd/hycommon/sampler.h"
#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/ucc/hyucc/model/non_ucc_list.h"
#include "core/config/thread_number/type.h"

namespace algos::hyucc {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, config::ThreadNumType threads = 1)
        : sampler_(std::move(plis), std::move(pli_records), threads) {}

    NonUCCList GetNonUCCs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::hyucc
