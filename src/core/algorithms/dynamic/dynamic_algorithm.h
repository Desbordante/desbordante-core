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

protected:
    // algo doing its stuff
    virtual unsigned long long ProcessBatch() = 0;
    void LoadDataInternal() override;
    unsigned long long ExecuteInternal() final;
    void MakeExecuteOptsAvailable() override;

    void ResetState() override;

    // modify statements in last batch of changes
    RowsContainer insert_statements_;
    std::vector<size_t> delete_statements_{};

    // init state of table
    InputTable input_table_;
    // insert operations stream
    InputTable insert_batch_;
    // delete operations stream
    std::vector<size_t> delete_batch_{};
    // update operations stream (old values)
    std::vector<size_t> update_old_batch_{};
    // update operations stream (new values)
    InputTable update_new_batch_;
    // stores current table rows ids (after last Execute() method call)
    std::unordered_set<size_t> table_rows_ids_{};

private:
    void AddSpecificNeededOptions(
            std::unordered_set<std::string_view>& previous_options) const override;
    void ValidateInsertStatements(InputTable& data);
    void ValidateDeleteStatements(std::vector<size_t>& data);
    void ConfigureBatch();
    void RegisterOptions();
    bool HasBatch();

    bool is_initialized_ = false;
};

}  // namespace algos