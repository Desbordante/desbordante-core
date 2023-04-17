#pragma once

#include <numeric>
#include <utility>
#include <vector>

#include <boost/dynamic_bitset.hpp>

namespace algos::hy {

/**
 * Collection of column combinations found by the Sampler module.
 * In the HyFD algorithm column combination repesents a non-FD.
 * In the HyUCC algorithm column combination repesents a non-UCC.
 *
 * Stores combinations with the same column count as a separate level.
 */
class ColumnCombinationList {
private:
    std::vector<std::vector<boost::dynamic_bitset<>>> ccs_;
    unsigned depth_ = 0;

public:
    explicit ColumnCombinationList(size_t num_attributes) : ccs_(num_attributes + 1) {}

    /**
     * Adds the column combination to the correspondent level. Does not check whether the level
     * already contains such a combination.
     *
     * @param column_set column combination to add.
     */
    void Add(boost::dynamic_bitset<>&& column_set) {
        unsigned level = column_set.count();

        ccs_[level].push_back(std::move(column_set));
        if (level > depth_) {
            depth_ = level;
        }
    }

    size_t GetNumAttributes() const noexcept {
        return ccs_.size() - 1;
    }

    /**
     * @return Highest level containing any number of combinations.
     */
    [[nodiscard]] unsigned GetDepth() const noexcept {
        return depth_;
    }

    /**
     * @return Collection of column combination with given column count.
     */
    [[nodiscard]] std::vector<boost::dynamic_bitset<>> const& GetLevel(size_t level) const {
        return ccs_.at(level);
    }

    /**
     * @return Total count of stored column combinations accross all levels.
     */
    [[nodiscard]] size_t GetTotalCount() const {
        return std::accumulate(ccs_.cbegin(), ccs_.cend(), 0,
                               [](size_t res, auto const& v) { return res + v.size(); });
    }
};

}  // namespace algos::hy
