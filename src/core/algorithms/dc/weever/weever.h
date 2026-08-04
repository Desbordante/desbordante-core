#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "core/algorithms/algorithm.h"
#include "core/algorithms/dc/model/dc.h"
#include "core/algorithms/dc/model/table_index.h"
#include "core/config/tabular_data/input_table_type.h"
#include "core/model/table/dynamic_data/dynamic_table_data.h"
#include "core/model/table/relational_schema.h"

namespace algos {

namespace dc {
using Violation = std::pair<size_t, size_t>;
}

class Weever final : public Algorithm {
private:
    dc::DC dc_;
    std::unique_ptr<RelationalSchema> relation_;
    std::unique_ptr<model::ITypedDynamicTableData> data_;
    std::unordered_map<size_t, roaring::Roaring> violations_;
    std::unordered_map<size_t, roaring::Roaring> inv_violations_;
    dc::TableIndex table_index_;
    size_t index_offset_ = 0;
    bool is_preprocessed_ = false;

    // Options
    std::string dc_string_;
    config::InputTable input_table_;
    config::InputTable insert_statements_table_;
    config::InputTable update_statements_table_;
    std::unordered_set<size_t> delete_statement_indices_;

    void Preprocess();
    void BuildIndices(dc::DC const& dc);

    // Dynamic update methods
    void InsertTuple(size_t tuple_id);
    void DeleteTuple(size_t tuple_id);
    void Update();

    void ResetState() override;
    void MakeExecuteOptsAvailable() override;

public:
    Weever();

    unsigned long long int ExecuteInternal() final;
    void LoadDataInternal() final override;
    void RegisterOptions();

    std::vector<dc::Violation> GetViolations() const;
    size_t GetViolationsSize() const;
};

}  // namespace algos
