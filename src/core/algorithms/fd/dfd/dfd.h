#pragma once

#include <random>
#include <stack>

#include "core/algorithms/fd/dfd/partition_storage/partition_storage.h"
#include "core/algorithms/fd/pli_based_fd_algorithm.h"
#include "core/config/thread_number/type.h"
#include "core/model/table/vertical.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    std::vector<Vertical> unique_columns_;

    config::ThreadNumType number_of_threads_;

    void MakeExecuteOptsAvailableFDInternal() final;
    void RegisterOptions();

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    DFD();
};

}  // namespace algos
