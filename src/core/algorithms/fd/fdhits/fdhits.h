#pragma once

#include <string>
#include <utility>
#include <vector>

#include "core/algorithms/fd/hycommon/types.h"
#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/config/fdhits_mode/type.h"

namespace algos::fd::fdhits {

class FDHits : public PliBasedFDAlgorithm {
private:
    void MakeExecuteOptsAvailableFDInternal() final;

    void ResetStateFd() final {}

    double sampling_factor_ = 0.3;
    config::FDHitsModeType mode_ = "union";

    unsigned long long ExecuteInternal() override;

    void RegisterFDs(std::vector<std::pair<std::vector<size_t>, size_t>> const& fds,
                     std::vector<hy::ClusterId> const& og_mapping);

public:
    FDHits();
};

}  // namespace algos::fd::fdhits
