#pragma once

#include "hycommon/sampler.h"
#include "hycommon/types.h"
#include "ucc/hyucc/structures/non_ucc_list.h"

namespace algos::hyucc {

class Sampler {
private:
    hy::Sampler sampler_;

public:
    Sampler(hy::PLIsPtr plis, hy::RowsPtr pli_records)
        : sampler_(std::move(plis), std::move(pli_records)) {}

    NonUCCList GetNonUCCs(hy::IdPairs const& comparison_suggestions) {
        return sampler_.GetAgreeSets(comparison_suggestions);
    }
};

}  // namespace algos::hyucc
