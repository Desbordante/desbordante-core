#include "dynamic_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/crud_operations/operations.h"

#include <easylogging++.h>


namespace {
using config::InputTable;

std::unordered_map<std::string, size_t> GetIndexes(const InputTable& table) {
    std::unordered_map<std::string, size_t> index;
    for (int i = 0; i < table->GetNumberOfColumns(); ++i) {
        index[table->GetColumnName(i)] = i;
    }
    return std::move(index);
}

std::vector<size_t> GetTransposition(const InputTable& table, const InputTable& stream) {
    static std::unordered_map<std::string, size_t> index = GetIndexes(table);
    std::vector<size_t> result;
    for (int i = 0; i < stream->GetNumberOfColumns(); ++i) {
        result.emplace_back(index[stream->GetColumnName(i)]);
    }
    return std::move(result);
}
} // namespace

void algos::DynamicAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kInsertStatementsOpt(&insert_statements_stream_));
    RegisterOption(config::kDeleteStatementsOpt(&delete_statements_stream_));
    RegisterOption(config::kUpdateOldStatementsOpt(&update_old_statements_stream_));
    RegisterOption(config::kUpdateNewStatementsOpt(&update_new_statements_stream_));
}

algos::DynamicAlgorithm::DynamicAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(phase_names) {
    RegisterOptions();
    MakeOptionsAvailable({config::names::kTable});
}

void algos::DynamicAlgorithm::LoadDataInternal() {
    if (input_table_->GetNumberOfColumns() == 0) {
        throw std::runtime_error("Unable to work on an empty dataset.");
    }
    is_initialized_ = false;
}

void algos::DynamicAlgorithm::ResetState() {
    insert_statements_.Clear();
    delete_statements_.Clear();
    table_validation_rows_.Clear();
    while (input_table_->HasNextRow()) {
        table_validation_rows_.Add(input_table_->GetNextRow());
    }
    input_table_->Reset();
}

void algos::DynamicAlgorithm::MakeExecuteOptsAvailable() {
    if (is_initialized_) {
        MakeOptionsAvailable(CRUD_OPTIONS);
    }
}

void algos::DynamicAlgorithm::Initialize() {
    LoadData();
    Execute();
    is_initialized_ = true;
    MakeExecuteOptsAvailable();
}

void algos::DynamicAlgorithm::ExtractAndValidate(RowsContainer &statements, 
                                                 InputTable &data_stream) {
    if (!data_stream) {
        return;
    }
    if (data_stream->GetNumberOfColumns() != input_table_->GetNumberOfColumns()) {
        throw config::ConfigurationError("Invalid data received: the number of columns in the \
            modification statements is different from the table.");
    }
    bool is_delete_statement = (&statements == &delete_statements_);
    while (data_stream->HasNextRow()) {
        TableRow row{data_stream->GetNextRow()};
        if (row.getData().size() != input_table_->GetNumberOfColumns()) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                            << input_table_->GetNumberOfColumns() << ", got " 
                            << row.getData().size() << ")";
            continue;
        }
        if (is_delete_statement) {
            if (!table_validation_rows_.Contains(row)) {
                throw config::ConfigurationError("Invalid data received: the row from the delete \
                    statement does not exist in the table. Bad statement: " + row.toString());
            } else {
                table_validation_rows_.Erase(row);
            }
        } else {
            table_validation_rows_.Add(TableRow(row));
        }
        statements.Add(row);
    }
}

void algos::DynamicAlgorithm::ConfigureOperations() {
    // configure insert statements
    ExtractAndValidate(insert_statements_, insert_statements_stream_);
    // configure delete statements
    ExtractAndValidate(delete_statements_, delete_statements_stream_);
    // configure update statements
    const size_t inserts_cnt = insert_statements_.Size();
    const size_t deletes_cnt = delete_statements_.Size();
    ExtractAndValidate(delete_statements_, update_old_statements_stream_);
    ExtractAndValidate(insert_statements_, update_new_statements_stream_);
    if (insert_statements_.Size() - inserts_cnt != delete_statements_.Size() - deletes_cnt) {
        throw config::ConfigurationError("Invalid data received: number of rows to update: "
            + std::to_string(insert_statements_.Size() - inserts_cnt)
            + ", number of rows to update with: "
            + std::to_string(delete_statements_.Size() - deletes_cnt));
    }
}

bool algos::DynamicAlgorithm::CheckRecievedBatch() {
    return insert_statements_stream_->HasNextRow() ||
           delete_statements_stream_->HasNextRow() ||
           update_old_statements_stream_->HasNextRow() ||
           update_new_statements_stream_->HasNextRow();
}

unsigned long long algos::DynamicAlgorithm::ProcessBatch() {
    if (CheckRecievedBatch()) {
        ConfigureOperations();
        unsigned long long time_ms = ProcessBatchInternal();
        UpdateResult();
        ClearOptions();
        MakeExecuteOptsAvailable();
        return time_ms;
    }
    return 0;
}
