#include "core/algorithms/ind/ind_algorithm.h"

#include "core/config/names_and_descriptions.h"
#include "core/config/tabular_data/input_tables/option.h"

namespace algos {

INDAlgorithm::INDAlgorithm() : Algorithm() {
    RegisterOption(config::kTablesOpt(&input_tables_));
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void INDAlgorithm::LoadDataInternal() {
    schemas_ = std::make_shared<std::vector<std::unique_ptr<RelationalSchema>>>();
    for (auto const& input_table : input_tables_) {
        schemas_->push_back(RelationalSchema::CreateFrom(*input_table));
    }

    LoadINDAlgorithmDataInternal();
}

}  // namespace algos
