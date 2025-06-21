#pragma once

#include <unordered_map>
#include <vector>

#include "algorithms/md/hymd/indexes/global_value_identifier.h"
#include "algorithms/md/hymd/indexes/pli_cluster.h"
#include "algorithms/md/hymd/table_identifiers.h"
#include "util/desbordante_assume.h"

namespace algos::hymd::indexes {

class KeyedPositionListIndex {
private:
    std::unordered_map<GlobalValueIdentifier, ValueIdentifier> value_id_mapping_;
    std::vector<GlobalValueIdentifier> value_ids_;
    std::vector<PliCluster> clusters_;
    RecordIdentifier cur_record_id_ = 0;
    ValueIdentifier next_value_id_ = 0;

public:
    ValueIdentifier AddNextValue(GlobalValueIdentifier value);

    std::unordered_map<GlobalValueIdentifier, ValueIdentifier> const& GetMapping() const noexcept {
        DESBORDANTE_ASSUME(value_id_mapping_.size() == clusters_.size());
        return value_id_mapping_;
    }

    std::vector<GlobalValueIdentifier> const& GetValueIds() const noexcept {
        return value_ids_;
    }

    std::vector<PliCluster> const& GetClusters() const noexcept {
        return clusters_;
    }
};

}  // namespace algos::hymd::indexes
