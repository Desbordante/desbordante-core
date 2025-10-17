#include "nar_algorithm.h"

#include <stdexcept>
#include <utility>

#include "algorithm.h"
#include "common_option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "descriptions.h"
#include "names.h"
#include "names_and_descriptions.h"
#include "nar/nar.h"
#include "option.h"
#include "table/column_layout_typed_relation_data.h"
#include "table/idataset_stream.h"

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
    RegisterOption(Option{&minconf_, kMinimumConfidence, kDMinimumConfidence, 0.0});
    RegisterOption(Option{&minsup_, kMinimumSupport, kDMinimumSupport, 0.0});
}

void NARAlgorithm::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMinimumSupport, kMinimumConfidence});
}

}  // namespace algos
