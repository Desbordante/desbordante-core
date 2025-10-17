#include "ind_algorithm.h"

#include <cstddef>

#include "algorithm.h"
#include "config/names_and_descriptions.h"
#include "config/tabular_data/input_tables/option.h"
#include "names_and_descriptions.h"
#include "table/idataset_stream.h"
#include "table/relational_schema.h"

namespace algos {

INDAlgorithm::INDAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    RegisterOption(config::kTablesOpt(&input_tables_));
    MakeOptionsAvailable({config::kTablesOpt.GetName()});
}

void INDAlgorithm::LoadDataInternal() {
    schemas_ = std::make_shared<std::vector<RelationalSchema>>();
    for (auto const& input_table : input_tables_) {
        auto& schema = schemas_->emplace_back<std::string>(input_table->GetRelationName());
        for (size_t i{0}; i < input_table->GetNumberOfColumns(); ++i) {
            schema.AppendColumn(input_table->GetColumnName(i));
        }
    }

    LoadINDAlgorithmDataInternal();
}

}  // namespace algos
