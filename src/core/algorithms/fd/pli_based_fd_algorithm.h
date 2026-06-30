#pragma once

#include <optional>

#include "core/algorithms/fd/fd_algorithm.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_relation_data.h"

namespace algos {

class LegacyPliBasedFDAlgorithm : public FDAlgorithm {
    config::InputTable input_table_;

    void RegisterOptions();
    void LoadDataInternal() final;

protected:
    std::shared_ptr<LegacyColumnLayoutRelationData> relation_;

    LegacyColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e. after algorithm
        // execution
        assert(relation_ != nullptr);
        return *relation_;
    }

public:
    LegacyPliBasedFDAlgorithm();
};

}  // namespace algos
