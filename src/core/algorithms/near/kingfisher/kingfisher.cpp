#include "kingfisher.h"

#include <iostream>

#include "algorithms/near/near_discovery.h"
#include "candidate_prefix_tree.h"
#include "config/names_and_descriptions.h"
#include "model/table/position_list_index.h"
#include "option_using.h"
#include "util/vector_to_string.h"

namespace algos {

Kingfisher::Kingfisher() : NeARDiscovery({}) {
    RegisterOptions();
}

void Kingfisher::RegisterOptions() {
    DESBORDANTE_OPTION_USING;

    RegisterOption(Option{&max_rules_, kMaxRules, kDMaxRules, 10u});
    RegisterOption(Option{&max_p_, kMaxPValue, kDMaxPValue, 1.2e-8});
}

void Kingfisher::ResetState() {
    // TODO: implement this
}

void Kingfisher::MakeExecuteOptsAvailable() {
    using namespace config::names;
    MakeOptionsAvailable({kMaxPValue, kMaxRules});
}

unsigned long long Kingfisher::ExecuteInternal() {
    auto tree = kingfisher::CandidatePrefixTree{max_p_, max_rules_, transactional_data_};
    tree.Explore();
    near_collection_ = tree.GetNeARIDs();
    return 0;
}

std::string Kingfisher::GetTreeHistory() {
    auto tree = kingfisher::CandidatePrefixTree{max_p_, max_rules_, transactional_data_};
    tree.Explore();
    return tree.GetTreeHistory();
}

}  // namespace algos
