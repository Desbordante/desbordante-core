#include "demo_algo.h"
#include "config/names.h"
#include "config/ioption.h"

using config::names::kTable;
using config::InputTable;

algos::DynamicAlgorithmDemo::DynamicAlgorithmDemo(std::vector<std::string_view> phase_names)
    : DynamicAlgorithm(phase_names) {}

unsigned long long algos::DynamicAlgorithmDemo::ExecuteInternal() {
    auto start_time = std::chrono::system_clock::now();
    
    auto opt_values = GetOptValues();
    if (opt_values.find(kTable) != opt_values.end()) {
        config::OptValue val = opt_values.at(kTable);
        InputTable data_stream = boost::any_cast<InputTable>(val.value);
        while (data_stream->HasNextRow()) {
            TableRow row(TableRow(data_stream->GetNextRow()));
            result_collection_.Add(row.toString());
        }
    }
    sleep(1);

    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

unsigned long long algos::DynamicAlgorithmDemo::ProcessBatchInternal() {
    auto start_time = std::chrono::system_clock::now();

    for (TableRow row : insert_statements_.AsUnorderedMultiset()) {
        added_to_result_.Add(row.toString());
    }
    for (TableRow row : delete_statements_.AsUnorderedMultiset()) {
        erased_from_result_.Add(row.toString());
    }
    
    sleep(1);
    auto elapsed_milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - start_time);
    return elapsed_milliseconds.count();
}

void algos::DynamicAlgorithmDemo::UpdateResult() {
    for (std::string elem : added_to_result_.AsUnorderedMultiset()) {
        result_collection_.Add(elem);
    }
    for (std::string elem : erased_from_result_.AsUnorderedMultiset()) {
        result_collection_.Erase(elem);
    }
}

std::vector<std::string> algos::DynamicAlgorithmDemo::GetResult() {
    return result_collection_.AsVector();
}

std::pair<std::vector<std::string>, std::vector<std::string>> algos::DynamicAlgorithmDemo::GetResultDiff() {
    return std::make_pair(added_to_result_.AsVector(), erased_from_result_.AsVector());
}

void algos::DynamicAlgorithmDemo::ResetState() {
    DynamicAlgorithm::ResetState();
    result_collection_.Clear();
    added_to_result_.Clear();
    erased_from_result_.Clear();
}
