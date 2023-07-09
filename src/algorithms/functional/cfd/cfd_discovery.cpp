#include "cfd_discovery.h"

#include <iterator>
#include <thread>

#include "config/equal_nulls/option.h"
#include "config/names_and_descriptions.h"
#include "config/option_using.h"
#include "config/tabular_data/input_table/option.h"
#include "functional/cfd/util/cfd_output_util.h"
#include "functional/cfd/util/set_util.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

CFDDiscovery::CFDDiscovery(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    using namespace config::names;
    RegisterOptions();
    MakeOptionsAvailable({kTable, kEqualNulls, kCfdColumnsNumber, kCfdTuplesNumber});
}

CFDDiscovery::CFDDiscovery() : CFDDiscovery({kDefaultPhaseName}) {}

void CFDDiscovery::LoadDataInternal() {
    relation_ = CFDRelationData::CreateFrom(*input_table_, is_null_equal_null_, columns_number_,
                                            tuples_number_);

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

    RegisterOption(config::TableOpt(&input_table_));
    RegisterOption(Option{&columns_number_, kCfdColumnsNumber, kDCfdColumnsNumber, 0u});
    RegisterOption(Option{&tuples_number_, kCfdTuplesNumber, kDCfdTuplesNumber, 0u});
    RegisterOption(config::EqualNullsOpt(&is_null_equal_null_));
}

int CFDDiscovery::NrCfds() const {
    return (int)cfd_list_.size();
}

CFDList CFDDiscovery::GetCfds() const {
    return cfd_list_;
}

std::string CFDDiscovery::GetCfdString(CFD const& cfd) const {
    return Output::CFDToString(cfd, relation_);
}

std::string CFDDiscovery::GetRelationString(char delim) const {
    return relation_->GetStringFormat(delim);
}

std::string CFDDiscovery::GetRelationString(const SimpleTIdList& subset, char delim) const {
    return relation_->GetStringFormat(subset, delim);
}
}  // namespace algos::cfd
