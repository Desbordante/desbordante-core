#pragma once

#include "core/algorithms/fd/afd_algorithm.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_relation_data.h"

namespace algos {

class PliBasedAFDAlgorithm : public AFDAlgorithm {
private:
    config::InputTable input_table_;

    void RegisterOptions();
    void LoadDataInternal() final;

protected:
    std::shared_ptr<ColumnLayoutRelationData> relation_;

public:
    PliBasedAFDAlgorithm();
};

}  // namespace algos
