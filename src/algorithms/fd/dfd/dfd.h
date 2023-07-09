#pragma once

#include <random>
#include <stack>

#include "algorithms/fd/dfd/partition_storage/partition_storage.h"
#include "algorithms/fd/pli_based_fd_algorithm.h"
#include "model/vertical.h"
#include "util/config/thread_number/type.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    std::vector<Vertical> unique_columns_;

    util::config::ThreadNumType number_of_threads_;

    void MakeExecuteOptsAvailable() final;
    void RegisterOptions();

    void ResetStateFd() final;
    unsigned long long ExecuteInternal() final;

public:
    DFD();
};

}  // namespace algos
