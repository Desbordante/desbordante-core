#include "algorithms/cfd/cfd_discovery.h"

// see ./LICENSE

#include <iterator>
#include <thread>

#include "algorithms/cfd/util/cfd_output_util.h"
#include "algorithms/cfd/util/set_util.h"
#include "algorithms/options/equal_nulls/option.h"
#include "algorithms/options/names_and_descriptions.h"

namespace algos {

CFDDiscovery::CFDDiscovery(std::vector<std::string_view> phase_names)
    : Primitive(std::move(phase_names)) {
    using namespace config::names;

    RegisterOptions();
    MakeOptionsAvailable({kEqualNulls, kCfdColumnsNumber, kCfdTuplesNumber});
}

CFDDiscovery::CFDDiscovery() : CFDDiscovery({kDefaultPhaseName}) {}

void CFDDiscovery::FitInternal(model::IDatasetStream& data_stream) {
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
    using namespace config::names;
    using namespace config::descriptions;
    using config::Option;

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

std::string CFDDiscovery::GetRelationString(const SimpleTidList& subset, char delim) const {
    return relation_->GetStringFormat(subset, delim);
}

}  // namespace algos
