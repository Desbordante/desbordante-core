#pragma once

#include "hycommon/sampler.h"
#include "hycommon/types.h"
#include "ucc/hyucc/structures/non_ucc_list.h"
#include "util/config/thread_number/type.h"

namespace algos::hyucc {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records, util::config::ThreadNumType threads = 1)
        : sampler_(std::move(plis), std::move(pli_records), threads) {}

    NonUCCList GetNonUCCs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::hyucc
