#pragma once

#include <boost/dynamic_bitset.hpp>
#include <numeric>
#include <vector>

namespace algos::hyfd {

/**
 * Collection of fd-violating column combinations found by the Sampler.
 *
 * Stores combinations with the same column count as a separate level.
 */
class NonFDList {
private:
    std::vector<std::vector<boost::dynamic_bitset<>>> fds_;
    unsigned depth_ = 0;

public:
    explicit NonFDList(size_t num_attributes) noexcept : fds_(num_attributes + 1) {}

    NonFDList(NonFDList&& other) = default;
    NonFDList& operator=(NonFDList&& other) = default;

    /**
     * Adds the column combination to the correspondent level. Does not check whether the level
     * already contains such a combination.
     *
     * @param column_set column combination to add.
     */
    void Add(boost::dynamic_bitset<>&& column_set) {
        unsigned level = column_set.count();

        fds_[level].push_back(std::move(column_set));
        if (level > depth_) {
            depth_ = level;
        }
    }

    /**
     * @return Highest level containing any number of combinations.
     */
    [[nodiscard]] unsigned GetDepth() const {
        return depth_;
    }

    /**
     * @return Collection of column combination with given column count.
     */
    [[nodiscard]] std::vector<boost::dynamic_bitset<>> const& GetLevel(size_t level) const {
        return fds_.at(level);
    }

    /**
     * @return Total count of column combination accross all levels.
     */
    [[nodiscard]] size_t GetTotalCount() const {
        return std::accumulate(fds_.cbegin(), fds_.cend(), 0,
                               [](size_t res, auto const& v) { return res + v.size(); });
    }
};

}  // namespace algos::hyfd