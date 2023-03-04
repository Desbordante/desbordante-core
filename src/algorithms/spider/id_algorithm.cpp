#include "id_algorithm.h"

#include "algorithms/options/descriptions.h"
#include "algorithms/options/names.h"

namespace algos {
std::vector<std::filesystem::path> IDAlgorithm::GetPathsFromData(
        std::filesystem::path const& data) {
    if (std::filesystem::is_regular_file(data)) {
        return {data};
    }

    std::vector<std::filesystem::path> paths;
    for (const auto& entry : std::filesystem::directory_iterator(data)) {
        paths.emplace_back(entry);
    }
    return paths;
}
decltype(IDAlgorithm::SepOpt) IDAlgorithm::SepOpt{
        {config::names::kSeparator, config::descriptions::kDSeparator}};

decltype(IDAlgorithm::HasHeaderOpt) IDAlgorithm::HasHeaderOpt{
        {config::names::kHasHeader, config::descriptions::kDHasHeader}};

decltype(IDAlgorithm::DataOpt) IDAlgorithm::DataOpt{
        {config::names::kData, config::descriptions::kDData}};

void IDAlgorithm::RegisterOptions() {
    RegisterOption(SepOpt.GetOption(&separator_));
    RegisterOption(HasHeaderOpt.GetOption(&has_header_));
    RegisterOption(DataOpt.GetOption(&data_));
}

}  // namespace algos
