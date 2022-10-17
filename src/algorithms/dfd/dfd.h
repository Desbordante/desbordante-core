#pragma once

#include <random>
#include <stack>

#include "pli_based_fd_algorithm.h"
#include "vertical.h"
#include "dfd/partition_storage/partition_storage.h"

namespace algos {

class DFD : public PliBasedFDAlgorithm {
private:
    std::unique_ptr<PartitionStorage> partition_storage_;
    std::vector<Vertical> unique_columns_;

    unsigned int number_of_threads_;

    unsigned long long ExecuteInternal() override;

public:
    explicit DFD(Config const& config);
    explicit DFD(std::shared_ptr<ColumnLayoutRelationData> relation, Config const& config);
};

}  // namespace algos
