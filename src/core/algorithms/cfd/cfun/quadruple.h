#pragma once

#include <cstddef>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::cfd::cfun {

class Quadruple {
public:
    using AttributeSet = boost::dynamic_bitset<>;
    using Partition = std::vector<std::vector<unsigned int>>;

private:
    struct ClusterData {
        std::vector<unsigned int> indices;
        AttributeSet closure;
        AttributeSet quasi_closure;
    };

    std::vector<ClusterData> clusters_;
    AttributeSet attribute_;

public:
    Quadruple(Partition partitions, AttributeSet a) : attribute_(std::move(a)) {
        clusters_.reserve(partitions.size());
        for (auto& p : partitions) {
            clusters_.push_back({std::move(p), attribute_, attribute_});
        }
    }

    Quadruple(Quadruple&& other) noexcept = default;
    Quadruple& operator=(Quadruple&& other) noexcept = default;

    std::vector<ClusterData> const& GetClusters() const noexcept {
        return clusters_;
    }

    AttributeSet const& GetAttributes() const noexcept {
        return attribute_;
    }

    bool IsEmptySets() const noexcept {
        return clusters_.empty();
    }

    std::vector<unsigned int> CalculateProbingTable(size_t num_rows) const;
    Quadruple Intersect(Quadruple const& other, size_t num_rows, size_t min_support) const;
    bool HasSameBegin(Quadruple const& other) const noexcept;
    std::vector<size_t> CheckCFD(unsigned int target_col,
                                 std::vector<unsigned int> const& target_col_ec) const;
    void PruneRedundantSets(Quadruple const& subset, std::vector<size_t> const& valid_cluster_ids,
                            size_t num_rows);
    void UpdateClosure(std::vector<size_t> const& valid_cluster_ids, unsigned int attribute_num);
    void UpdateQuasiClosure(std::vector<Quadruple const*> const& subsets, size_t num_rows);
};
}  // namespace algos::cfd::cfun
