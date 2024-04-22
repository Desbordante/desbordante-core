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
using config::InputTable;

class DynamicAlgorithm : public Algorithm {
public:
    explicit DynamicAlgorithm(std::vector<std::string_view> phase_names);
    
    // initialize and run static algo
    void Initialize();
    // processing batch
    unsigned long long ProcessBatch();

protected:
    // algo doing its stuff
    virtual unsigned long long ProcessBatchInternal() = 0;
    virtual void UpdateResult() {};
    void MakeExecuteOptsAvailable() override;

    virtual void ResetState();

    // modify statements in last batch of changes
    RowsContainer insert_statements_;
    RowsContainer delete_statements_;

    // init state of table
    InputTable input_table_;
    // insert operations stream
    InputTable insert_statements_stream_;
    // delete operations stream
    InputTable delete_statements_stream_;
    // update operations stream (old values)
    InputTable update_old_statements_stream_;
    // update operations stream (new values)
    InputTable update_new_statements_stream_;

private:
    void ExtractAndValidate(RowsContainer &statements, InputTable &data_stream);
    void ConfigureOperations();
    void RegisterOptions();
    bool CheckRecievedBatch();
    void LoadDataInternal() final;

    // stores current table state after processing all current delete operations. 
    // fills after execute method with table init data after static algo execution 
    // and insert_statements_ after processing every batch
    RowsContainer table_validation_rows_ {};
};

}  // namespace algos