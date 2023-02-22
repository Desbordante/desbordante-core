#pragma once

#include <random>
#include <stack>

#include "algorithms/dfd/partition_storage/partition_storage.h"
#include "algorithms/options/thread_number_opt.h"
#include "algorithms/pli_based_fd_algorithm.h"
#include "model/vertical.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    std::unique_ptr<PartitionStorage> partition_storage_;
    std::vector<Vertical> unique_columns_;

    config::ThreadNumType number_of_threads_;

    void MakeExecuteOptsAvailable() final;
    void RegisterOptions();

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    DFD();
};

}  // namespace algos
