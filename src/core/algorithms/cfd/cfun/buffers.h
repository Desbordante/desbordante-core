#pragma once

#include <algorithm>
#include <core/algorithms/cfd/cfun/quadruple.h>
#include <cstddef>
#include <vector>

namespace algos::cfd::cfun {
struct IntersectBuffers {
    std::vector<unsigned int> assoc;
    std::vector<unsigned int> counts;
    std::vector<unsigned int> elem_result;

    IntersectBuffers() noexcept = default;

    IntersectBuffers(size_t num_rows)
        : assoc(num_rows + 1, 0), counts(num_rows + 1, 0), elem_result(num_rows + 1, 0) {}

    void Reset() {
        std::fill(assoc.begin(), assoc.end(), 0);
        std::fill(counts.begin(), counts.end(), 0);
        std::fill(elem_result.begin(), elem_result.end(), 0);
    }

    void Clear() {
        assoc.clear();
        counts.clear();
        elem_result.clear();
    }
};

struct ProbingBuffer {
    std::vector<unsigned int> probing_table;

    ProbingBuffer() noexcept = default;

    ProbingBuffer(size_t num_rows) : probing_table(num_rows, 0) {}

    void CalculateProbingTable(std::vector<Quadruple::ClusterData> const& clusters) {
        unsigned int next_cluster_id = 1;
        for (auto const& cluster : clusters) {
            for (auto tuple : cluster.indices) {
                probing_table[tuple] = next_cluster_id;
            }
            ++next_cluster_id;
        }
    }

    void Reset() {
        std::fill(probing_table.begin(), probing_table.end(), 0);
    }

    void Clear() {
        probing_table.clear();
    }
};
}  // namespace algos::cfd::cfun