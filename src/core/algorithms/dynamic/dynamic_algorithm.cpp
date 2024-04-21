#include "dynamic_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "config/names.h"

#include <easylogging++.h>

using config::InputTable;
using config::names::kTable;

void algos::DynamicAlgorithm::ConfigureOperations() {
    // process insert statements
    ExtractAndValidate(insert_statements_, config::names::kInsertStatements);
    // process delete statements
    ExtractAndValidate(delete_statements_, config::names::kDeleteStatements);
    // process update statements
    size_t inserts_cnt = insert_statements_.Size();
    size_t deletes_cnt = delete_statements_.Size();
    ExtractAndValidate(delete_statements_, config::names::kUpdateOldStatements);
    ExtractAndValidate(insert_statements_, config::names::kUpdateNewStatements);
    if (insert_statements_.Size() - inserts_cnt != delete_statements_.Size() - deletes_cnt) {
        throw config::ConfigurationError("Invalid data received: number of rows to update: "
            + std::to_string(insert_statements_.Size() - inserts_cnt)
            + ", number of rows to update with: "
            + std::to_string(delete_statements_.Size() - deletes_cnt));
    }
}

std::vector<std::string_view> algos::DynamicAlgorithm::GetOperationsOptions() {
    static std::vector<std::string_view> options{config::names::kInsertStatements, 
                                                 config::names::kDeleteStatements,
                                                 config::names::kUpdateOldStatements,
                                                 config::names::kUpdateNewStatements};
    return options;
}

void algos::DynamicAlgorithm::ExtractAndValidate(RowsContainer &statements, std::string_view option_name) {
    auto opt_values = GetOptValues();
    bool is_delete_statements = (&statements == &delete_statements_);
    if (opt_values.find(option_name) != opt_values.end()) {
        InputTable data_stream = boost::any_cast<InputTable>(opt_values.at(option_name).value);
        if (data_stream->GetNumberOfColumns() != table_cols_cnt_) {
            throw config::ConfigurationError("Invalid data received: the number of columns in the \
                modification statements is different from the table.");
        }
        while (data_stream->HasNextRow()) {
            TableRow row(TableRow(data_stream->GetNextRow()));
            if (row.getData().size() != table_cols_cnt_) {
                LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                             << table_cols_cnt_ << ", got " << row.getData().size() << ")";
                continue;
            }
            if (!table_validation_rows_.Contains(row)) {
                throw config::ConfigurationError("Invalid data received: the row from the delete \
                    statement does not exist in the table. Bad statement: " + row.toString());
            } else {
                table_validation_rows_.Erase(row);
                statements.Add(row);
            }
        }
    }
    UnsetOption(option_name);
}

algos::DynamicAlgorithm::DynamicAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(phase_names) {}

unsigned long long algos::DynamicAlgorithm::ProcessBatch() {
    ConfigureOperations();
    unsigned long long time_ms = ProcessBatchInternal();
    return time_ms;
}

void algos::DynamicAlgorithm::LoadDataInternal() {
    static_algo_executed_ = false;
    auto opt_values = GetOptValues();
    RowsContainer statements{};
    if (opt_values.find(kTable) != opt_values.end()) {
        table_cols_cnt_ = boost::any_cast<InputTable>(opt_values.at(kTable).value)->GetNumberOfColumns();
    }
}

void algos::DynamicAlgorithm::ResetState() {
    insert_statements_.Clear();
    delete_statements_.Clear();
    table_validation_rows_.Clear();
    table_cols_cnt_ = 0;
    static_algo_executed_ = false;
    auto opt_values = GetOptValues();
    if (opt_values.find(kTable) != opt_values.end()) {
        boost::any_cast<InputTable>(opt_values.at(kTable).value)->Reset();
    }
}

unsigned long long algos::DynamicAlgorithm::ExecuteInternal() {
    return 0;
}

unsigned long long algos::DynamicAlgorithm::ProcessBatchInternal() {
    return 0;
}

void algos::DynamicAlgorithm::UpdateResult() {}
