#pragma once

#include <memory>

#include "core/algorithms/algorithm.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/position_list_index.h"
#include "core/model/table/table_header.h"

namespace algos {
// A few algorithms share the same LoadDataInternal method, this class is here purely to avoid code
// repetition.
class PartitionOnlyAlgorithm : public Algorithm {
    config::InputTable input_table_;

    void RegisterOptions();
    void LoadDataInternal() final;

protected:
    model::TableHeader table_header_;
    std::vector<model::PositionListIndex> stripped_partitions_;

    PartitionOnlyAlgorithm();
};
}  // namespace algos
