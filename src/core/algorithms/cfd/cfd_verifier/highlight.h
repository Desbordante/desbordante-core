#pragma once

#include "model/table/position_list_index.h"

namespace algos::cfd_verifier {

class Highlight {
private:
    model::PLI::Cluster cluster_;
    std::vector<size_t> violating_rows_;

public:
    Highlight(model::PLI::Cluster const& cluster, std::vector<size_t> violating_rows)
        : cluster_(cluster), violating_rows_(violating_rows) {}

    model::PLI::Cluster const& GetCluster() const {
        return cluster_;
    }

    std::vector<size_t> GetViolatingRows() const {
        return violating_rows_;
    }
};
}  // namespace algos::cfd_verifier
