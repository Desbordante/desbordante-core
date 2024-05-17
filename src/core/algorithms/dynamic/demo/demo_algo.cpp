#include "demo_algo.h"

#include "config/ioption.h"
#include "config/names.h"

using config::InputTable;
using config::names::kTable;

algos::DynamicAlgorithmDemo::DynamicAlgorithmDemo(std::vector<std::string_view> phase_names)
    : DynamicAlgorithm(phase_names) {}

unsigned long long algos::DynamicAlgorithmDemo::ProcessBatch() {
    auto start_time = std::chrono::system_clock::now();
    added_to_result_.Clear();
    erased_from_result_.Clear();
    for (size_t row_id : delete_statements_) {
        erased_from_result_.Add(result_collection_.Erase({row_id}));
    }
    for (TableRow row : insert_statements_.AsUnorderedMultiset()) {
        table_rows_ids_.emplace(row.getId());
        result_collection_.Add(row);
        added_to_result_.Add(row);
    }
    sleep(1);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void algos::DynamicAlgorithmDemo::LoadDataInternal() {
    DynamicAlgorithm::LoadDataInternal();
    while (input_table_->HasNextRow()) {
        TableRow row{input_table_->GetNextRow()};
        table_rows_ids_.emplace(row.getId());
        result_collection_.Add(row);
    }
    sleep(1);
}

std::vector<std::string> algos::DynamicAlgorithmDemo::GetResult() {
    return result_collection_.AsStringVector();
}

std::pair<std::vector<std::string>, std::vector<std::string>> 
algos::DynamicAlgorithmDemo::GetResultDiff() {
    return std::make_pair(added_to_result_.AsStringVector(), erased_from_result_.AsStringVector());
}
