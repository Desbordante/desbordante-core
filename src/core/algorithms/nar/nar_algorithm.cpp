#include "core/algorithms/nar/nar_algorithm.h"

#include "core/config/ar_minimum_conf/option.h"
#include "core/config/ar_minimum_support/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

namespace algos {

NARAlgorithm::NARAlgorithm(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable});
}

void NARAlgorithm::LoadDataInternal() {
    typed_relation_ = model::ColumnLayoutTypedRelationData::CreateFrom(*input_table_, true);
    input_table_->Reset();
    if (typed_relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty dataset: Numeric AR mining is meaningless.");
    }
}

void NARAlgorithm::ResetState() {
    nar_collection_.clear();
}

void NARAlgorithm::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
    RegisterOption(config::kArMinimumConfidenceOpt(&minconf_));
    RegisterOption(config::kArMinimumSupportOpt(&minsup_));
}

void NARAlgorithm::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kArMinimumSupport, kArMinimumConfidence});
}

}  // namespace algos
