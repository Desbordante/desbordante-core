#pragma once

#include <optional>

#include "core/algorithms/fd/afd_algorithm.h"
#include "core/config/equal_nulls/type.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_layout_relation_data.h"

namespace algos {

class PliBasedAFDAlgorithm : public AFDAlgorithm {
private:
    config::InputTable input_table_;
    config::EqNullsType is_null_equal_null_;

    void RegisterOptions();

protected:
    std::shared_ptr<ColumnLayoutRelationData> relation_;

    void LoadDataInternal() final;

    ColumnLayoutRelationData const& GetRelation() const noexcept {
        // GetRelation should be called after the dataset has been parsed, i.e. after algorithm
        // execution
        assert(relation_ != nullptr);
        return *relation_;
    }

public:
    PliBasedAFDAlgorithm(std::vector<std::string_view> phase_names);
};

}  // namespace algos
