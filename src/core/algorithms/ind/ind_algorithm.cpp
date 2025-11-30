#include "core/algorithms/ind/ind_algorithm.h"

#include "core/config/names_and_descriptions.h"
#include "core/config/tabular_data/input_tables/option.h"

namespace algos {

INDAlgorithm::INDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOption(config::kTablesOpt(&input_tables_));
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void INDAlgorithm::LoadDataInternal() {
    schemas_ = std::make_shared<std::vector<std::unique_ptr<RelationalSchema>>>();
    for (auto const& input_table : input_tables_) {
        auto schema = std::make_unique<RelationalSchema>(input_table->GetRelationName());
        for (size_t i{0}; i < input_table->GetNumberOfColumns(); ++i) {
            schema->AppendColumn(input_table->GetColumnName(i));
        }
        schemas_->push_back(std::move(schema));
    }

    LoadINDAlgorithmDataInternal();
}

}  // namespace algos
