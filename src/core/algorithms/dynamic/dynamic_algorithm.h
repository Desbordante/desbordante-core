#pragma once

#include <filesystem>
#include <list>
#include <mutex>

#include "algorithms/algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/table_row.h"
#include "util/dynamic_collection.h"

using RowsContainer = util::DynamicCollection<TableRow>;

namespace algos {

class DynamicAlgorithm : public Algorithm {
public:
    explicit DynamicAlgorithm(std::vector<std::string_view> phase_names);
    
    unsigned long long ProcessBatch();
    std::vector<std::string_view> GetOperationsOptions();

protected:
    // algo doing its stuff
    virtual unsigned long long ExecuteInternal();
    virtual unsigned long long ProcessBatchInternal();
    void LoadDataInternal() override;
    virtual void UpdateResult();

    virtual void ResetState();

    // modify statements in last batch of changes
    RowsContainer insert_statements_;
    RowsContainer delete_statements_;

private:
    void ConfigureOperations();
    void ExtractAndValidate(RowsContainer &statements, std::string_view option_name);

    // stores current table state after processing all current delete operations. 
    // fills after execute method with table init data after static algo execution 
    // and insert_statements_ after processing every batch
    RowsContainer table_validation_rows_;
    int table_cols_cnt_ = 0;
    bool static_algo_executed_ = false;
};

}  // namespace algos