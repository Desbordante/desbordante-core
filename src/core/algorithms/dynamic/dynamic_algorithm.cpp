#include "dynamic_algorithm.h"

#include <easylogging++.h>

#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/crud_operations/operations.h"
#include "config/tabular_data/input_table/option.h"
#include "config/tabular_data/input_table_type.h"


void algos::DynamicAlgorithm::RegisterOptions() {
    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kInsertStatementsOpt(&insert_batch_));
    RegisterOption(config::kDeleteStatementsOpt(&delete_batch_));
    RegisterOption(config::kUpdateOldStatementsOpt(&update_old_batch_));
    RegisterOption(config::kUpdateNewStatementsOpt(&update_new_batch_));
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
    is_initialized_ = true;
}

void algos::DynamicAlgorithm::ResetState() {
    insert_statements_.Clear();
    delete_statements_.clear();
}

void algos::DynamicAlgorithm::MakeExecuteOptsAvailable() {
    if (is_initialized_) {
        MakeOptionsAvailable(kCrudOptions);
    }
}

void algos::DynamicAlgorithm::AddSpecificNeededOptions(
        std::unordered_set<std::string_view>& previous_options) const {
    for (const std::string_view& option_name : kCrudOptions) {
        previous_options.erase(option_name);
    }
}

void algos::DynamicAlgorithm::ValidateInsertStatements(InputTable& data) {
    if (!data) {
        return;
    }
    if (data->GetNumberOfColumns() != input_table_->GetNumberOfColumns()) {
        throw config::ConfigurationError(
            "Invalid data received: the number of columns in the \
            modification statements is different from the table.");
    }
    while (data->HasNextRow()) {
        TableRow row{data->GetNextRow()};
        if (row.getData().size() != input_table_->GetNumberOfColumns()) {
            LOG(WARNING) << "Unexpected number of columns for a row, skipping (expected "
                         << input_table_->GetNumberOfColumns() << ", got " << row.getData().size()
                         << ")";
            continue;
        }
        insert_statements_.Add(row);
        table_rows_ids_.insert(row.getId());
    }
}

void algos::DynamicAlgorithm::ValidateDeleteStatements(std::vector<size_t>& data) {
    for (size_t row_id : data) {
        if (!table_rows_ids_.count(row_id)) {
            throw config::ConfigurationError("Invalid data received: the row with " +
                                             std::to_string(row_id) +
                                             " does not exist in the table.");
        } else {
            table_rows_ids_.erase(row_id);
            delete_statements_.emplace_back(row_id);
        }
    }
    data.clear();
}

void algos::DynamicAlgorithm::ConfigureBatch() {
    // configure update statements
    ValidateDeleteStatements(update_old_batch_);
    ValidateInsertStatements(update_new_batch_);
    if (insert_statements_.Size() != delete_statements_.size()) {
        throw config::ConfigurationError(
            "Invalid data received: number of rows to update: " +
            std::to_string(insert_statements_.Size()) +
            ", number of rows to update with: " + std::to_string(delete_statements_.size()));
    }
    // configure insert statements
    ValidateInsertStatements(insert_batch_);
    // configure delete statements
    ValidateDeleteStatements(delete_batch_);
}

bool algos::DynamicAlgorithm::HasBatch() {
    bool result = false;
    for (const std::string_view& option_name : kCrudOptions) {
        result |= IsOptionSet(option_name);
    }
    return result;
}

unsigned long long algos::DynamicAlgorithm::ExecuteInternal() {
    if (HasBatch()) {
        ConfigureBatch();
        unsigned long long time_ms = ProcessBatch();
        return time_ms;
    }
    return 0;
}
