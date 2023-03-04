#pragma once

#include <filesystem>

#include "algorithms/primitive.h"
#include "type.h"

namespace algos {

class IDAlgorithm : public Primitive {
    void RegisterOptions();

protected:
    std::filesystem::path data_;
    std::vector<std::filesystem::path> paths_;
    char separator_;
    bool has_header_;

    static config::OptionType<std::filesystem::path> DataOpt;
    static config::OptionType<decltype(separator_)> SepOpt;
    static config::OptionType<decltype(has_header_)> HasHeaderOpt;
    static std::vector<std::filesystem::path> GetPathsFromData(std::filesystem::path const& data);

public:
    explicit IDAlgorithm(std::vector<std::string_view> phase_names)
        : Primitive(std::move(phase_names)) {
        RegisterOptions();
    }

    void FitInternal(model::IDatasetStream& /*data_stream*/) final {}
};

}  // namespace algos
