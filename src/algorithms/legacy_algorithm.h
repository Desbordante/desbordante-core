#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

#include "algorithms/relational_algorithm.h"
#include "model/idataset_stream.h"
#include "parser/csv_parser.h"

namespace algos {

class LegacyAlgorithm : public RelationalAlgorithm {
private:
    void LoadDataInternal() override {}

    void ResetState() final {}

protected:
    std::unique_ptr<model::IDatasetStream> input_generator_;

public:
    constexpr static double kTotalProgressPercent = 100.0;

    LegacyAlgorithm(LegacyAlgorithm const& other) = delete;
    LegacyAlgorithm& operator=(LegacyAlgorithm const& other) = delete;
    LegacyAlgorithm(LegacyAlgorithm&& other) = delete;
    LegacyAlgorithm& operator=(LegacyAlgorithm&& other) = delete;
    virtual ~LegacyAlgorithm() override = default;

    explicit LegacyAlgorithm(std::vector<std::string_view> phase_names)
        : RelationalAlgorithm(std::move(phase_names)) {
        ExecutePrepare();
    }

    LegacyAlgorithm(std::unique_ptr<model::IDatasetStream> input_generator_ptr,
                    std::vector<std::string_view> phase_names)
        : RelationalAlgorithm(std::move(phase_names)),
          input_generator_(std::move(input_generator_ptr)) {
        ExecutePrepare();
    }

    LegacyAlgorithm(std::filesystem::path const& path, char const separator, bool const has_header,
                    std::vector<std::string_view> phase_names)
        : RelationalAlgorithm(std::move(phase_names)),
          input_generator_(std::make_unique<CSVParser>(path, separator, has_header)) {
        ExecutePrepare();
    }
};

}  // namespace algos
