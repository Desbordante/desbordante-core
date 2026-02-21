#pragma once

#include <memory>
#include <vector>

#include "core/algorithms/dd/dd.h"
#include "core/algorithms/dd/dd_algorithm.h"
#include "core/algorithms/dd/fastdd/model/differential_dependency.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/column_index.h"
#include "core/model/table/column_layout_typed_relation_data.h"
#include "core/model/types/builtin.h"

namespace algos::dd {

class FastDD final : public DDAlgorithm {
private:
    config::InputTable input_table_;

    std::shared_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    unsigned num_rows_;
    model::ColumnIndex num_columns_;
    unsigned shard_length_;

    std::vector<model::TypeId> type_ids_;

    config::InputTable difference_table_;
    std::shared_ptr<model::ColumnLayoutTypedRelationData> difference_typed_relation_;

    std::vector<DifferentialDependency> dds_;

    void RegisterOptions();
    void SetLimits();
    void CheckTypes();
    void ParseDifferenceTable();

    virtual void ResetStateDD() override {
        dds_.clear();
    }

protected:
    void LoadDataInternal() override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    FastDD();

    std::vector<DifferentialDependency> const& GetDDs() const {
        return dds_;
    }
};

}  // namespace algos::dd
