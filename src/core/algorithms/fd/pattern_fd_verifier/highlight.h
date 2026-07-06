#pragma once

#include "model/table/position_list_index.h"

namespace algos::pattern_fd {

class Highlight {
private:
    model::PLI::Cluster cluster_;
    std::vector<size_t> violating_rows_;

public:
    Highlight(model::PLI::Cluster cluster, std::vector<size_t> violating_rows)
        : cluster_(std::move(cluster)), violating_rows_(std::move(violating_rows)) {}

    model::PLI::Cluster const& GetCluster() const {
        return cluster_;
    }

    std::vector<size_t> const& GetViolatingRows() const {
        return violating_rows_;
    }
};
}  // namespace algos::pattern_fd