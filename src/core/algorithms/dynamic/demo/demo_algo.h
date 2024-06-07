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
    RowsContainer result_collection_;

    // result diff after last processBatch() call
    RowsContainer added_to_result_;
    RowsContainer erased_from_result_;

    unsigned long long ProcessBatch() final;
    void LoadDataInternal() final;

public:
    DynamicAlgorithmDemo(std::vector<std::string_view> phase_names = {});

    std::vector<std::string> GetResult();
    std::pair<std::vector<std::string>, std::vector<std::string>> GetResultDiff();
};

}  // namespace algos