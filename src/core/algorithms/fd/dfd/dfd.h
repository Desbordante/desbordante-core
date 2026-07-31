#pragma once

#include <boost/dynamic_bitset.hpp>

#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"
#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/config/thread_number/type.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    config::ThreadNumType number_of_threads_;

    void MakeExecuteOptsAvailableFDInternal() final;
    void RegisterOptions();

    void ResetStateFd() final;
    void ExecuteInternal() final;

public:
    DFD();
};

}  // namespace algos
