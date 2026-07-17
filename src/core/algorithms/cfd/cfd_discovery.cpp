#include "core/algorithms/cfd/cfd_discovery.h"

#include "core/algorithms/cfd/util/set_util.h"
#include "core/config/equal_nulls/option.h"
#include "core/config/names_and_descriptions.h"
#include "core/config/option_using.h"
#include "core/config/tabular_data/input_table/option.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

CFDDiscovery::CFDDiscovery() : Algorithm() {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable});
}

void CFDDiscovery::LoadDataInternal() {
    relation_ = CFDRelationData::CreateFrom(*input_table_);

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty .csv file: CFD mining is meaningless.");
    }
}

void CFDDiscovery::ResetState() {
    cfd_list_.clear();
    ResetStateCFD();
}

void CFDDiscovery::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(config::kTableOpt(&input_table_));
}

CFDList const& CFDDiscovery::GetCfds() const {
    return cfd_list_;
}
}  // namespace algos::cfd
