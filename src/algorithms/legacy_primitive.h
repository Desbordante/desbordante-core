#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>

#include "algorithms/primitive.h"
#include "model/idataset_stream.h"
#include "parser/csv_parser.h"

namespace algos {

class LegacyPrimitive : public CsvPrimitive {
private:
    void FitInternal([[maybe_unused]] model::IDatasetStream &data_stream) override {}

    void ResetState() final {}

protected:
    std::unique_ptr<model::IDatasetStream> input_generator_;

public:
    constexpr static double kTotalProgressPercent = 100.0;

    LegacyPrimitive(LegacyPrimitive const& other) = delete;
    LegacyPrimitive& operator=(LegacyPrimitive const& other) = delete;
    LegacyPrimitive(LegacyPrimitive&& other) = delete;
    LegacyPrimitive& operator=(LegacyPrimitive&& other) = delete;
    virtual ~LegacyPrimitive() override = default;

    explicit LegacyPrimitive(std::vector<std::string_view> phase_names)
        : CsvPrimitive(std::move(phase_names)) {
        ExecutePrepare();
    }

    LegacyPrimitive(std::unique_ptr<model::IDatasetStream> input_generator_ptr,
                    std::vector<std::string_view> phase_names)
        : CsvPrimitive(std::move(phase_names)), input_generator_(std::move(input_generator_ptr)) {
        ExecutePrepare();
    }

    LegacyPrimitive(std::filesystem::path const& path, char const separator, bool const has_header,
                    std::vector<std::string_view> phase_names)
        : CsvPrimitive(std::move(phase_names)),
          input_generator_(std::make_unique<CSVParser>(path, separator, has_header)) {
        ExecutePrepare();
    }
};

}  // namespace algos
