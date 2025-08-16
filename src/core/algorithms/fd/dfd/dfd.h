#pragma once

#include <optional>  // for optional, nullopt
#include <vector>    // for vector

#include "algorithms/fd/pli_based_fd_algorithm.h"  // for PliBasedFDAlgorithm
#include "config/thread_number/type.h"             // for ThreadNumType
#include "model/table/vertical.h"                  // for Vertical

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
    DFD(std::optional<ColumnLayoutRelationDataManager> relation_manager = std::nullopt);
};

}  // namespace algos
