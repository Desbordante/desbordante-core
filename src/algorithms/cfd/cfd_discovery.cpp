#include "algorithms/cfd/cfd_discovery.h"

// see ./LICENSE

#include <iterator>
#include <thread>

#include <easylogging++.h>

#include "algorithms/cfd/partition_table.h"
#include "algorithms/cfd/util/set_util.h"
#include "algorithms/cfd/util/cfd_output_util.h"
#include "algorithms/options/equal_nulls_opt.h"

namespace algos {

decltype(CFDDiscovery::ColumnsNumberOpt) CFDDiscovery::ColumnsNumberOpt{
        {config::names::kCfdColumnsNumber, config::descriptions::kDCfdColumnsNumber}, 0};

decltype(CFDDiscovery::TuplesNumberOpt) CFDDiscovery::TuplesNumberOpt{
        {config::names::kCfdTuplesNumber, config::descriptions::kDCfdTuplesNumber}, 0};

CFDDiscovery::CFDDiscovery(std::vector<std::string_view> phase_names)
    : Primitive(std::move(phase_names)) {
    RegisterOptions();
    MakeOptionsAvailable(config::GetOptionNames(config::EqualNullsOpt, TuplesNumberOpt, ColumnsNumberOpt));
}

CFDDiscovery::CFDDiscovery() : CFDDiscovery({kDefaultPhaseName}) {}

void CFDDiscovery::FitInternal(model::IDatasetStream& data_stream) {
    if (relation_ == nullptr) {
        relation_ =
                CFDRelationData::CreateFrom(data_stream, is_null_equal_null_,
                                            columns_number_, tuples_number_);
    }

    if (relation_->GetColumnData().empty()) {
        throw std::runtime_error("Got an empty .csv file: CFD mining is meaningless.");
    }
}

void CFDDiscovery::ResetState() {
    cfd_list_.clear();
    ResetStateCFD();
}

void CFDDiscovery::RegisterOptions() {
    RegisterOption(config::EqualNullsOpt.GetOption(&is_null_equal_null_));
    RegisterOption(TuplesNumberOpt.GetOption(&tuples_number_));
    RegisterOption(ColumnsNumberOpt.GetOption(&columns_number_));
}

[[maybe_unused]] int CFDDiscovery::NrCfds() const {
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


} //namespace algos
