#include "algorithms/cfd/cfd_discovery.h"

#include <iterator>
#include <thread>

#include "algorithms/cfd/util/cfd_output_util.h"
#include "algorithms/cfd/util/set_util.h"
#include "util/config/equal_nulls/option.h"
#include "util/config/names_and_descriptions.h"

// see algorithms/cfd/LICENSE

namespace algos::cfd {

CFDDiscovery::CFDDiscovery(std::vector<std::string_view> phase_names)
    : Algorithm(std::move(phase_names)) {
    using namespace util::config::names;

    RegisterOptions();
    MakeOptionsAvailable({kEqualNulls, kCfdColumnsNumber, kCfdTuplesNumber});
}

CFDDiscovery::CFDDiscovery() : CFDDiscovery({kDefaultPhaseName}) {}

void CFDDiscovery::LoadDataInternal(model::IDatasetStream& data_stream) {
    relation_ = CFDRelationData::CreateFrom(data_stream, is_null_equal_null_, columns_number_,
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
    using namespace util::config::names;
    using namespace util::config::descriptions;
    using util::config::Option;

    RegisterOption(Option{&columns_number_, kCfdColumnsNumber, kDCfdColumnsNumber, 0u});
    RegisterOption(Option{&tuples_number_, kCfdTuplesNumber, kDCfdTuplesNumber, 0u});
    RegisterOption(util::config::EqualNullsOpt(&is_null_equal_null_));
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
