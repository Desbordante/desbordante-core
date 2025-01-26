#include "cind_miner.hpp"

#include "ind/cind/condition_type.hpp"

namespace algos::cind {
CindMiner::CindMiner(std::shared_ptr<std::vector<model::ColumnDomain>> domains)
    : domains_(std::move(domains)), condition_type_(CondType::row) {}

void CindMiner::Execute(std::list<model::IND> const& /*aind_list*/) {
    fprintf(stderr, "validity: %lf, completeness: %lf, type: %s\n", precision_, recall_, condition_type_._to_string());
    // for (auto const& aind : aind_list) {
    //     ExecuteSingle(aind);
    // }
}
}  // namespace algos::cind