#pragma once

#include <filesystem>
#include <list>
#include <mutex>

#include "algorithms/dynamic/dynamic_algorithm.h"
#include "config/tabular_data/input_table_type.h"
#include "model/table/table_row.h"
#include "util/dynamic_collection.h"

namespace algos {

class DynamicAlgorithmDemo : public DynamicAlgorithm {
private:
    // algorithm result for the last version of data.
    util::DynamicCollection<std::string> result_collection_;

    // result diff after last processBatch() call
    util::DynamicCollection<std::string> added_to_result_;
    util::DynamicCollection<std::string> erased_from_result_;

    unsigned long long ExecuteInternal() final;
    unsigned long long ProcessBatchInternal() final;
    void UpdateResult() final;

    void ResetState() final;

public:
    DynamicAlgorithmDemo(std::vector<std::string_view> phase_names = {});

    std::vector<std::string> GetResult();
    std::pair<std::vector<std::string>, std::vector<std::string>> GetResultDiff();

};

}  // namespace algos