#pragma once

#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "non_fd_list.h"

namespace algos::hyfd {

/**
 * Collection of fd-violating column combinations found by the Sampler.
 *
 * Stores combinations found during the lifetime as well as combinations found since
 * the last access.
 */
class NonFds {
private:
    std::unordered_set<boost::dynamic_bitset<>> total_non_fds_;
    NonFDList new_non_fds_;

public:
    explicit NonFds(size_t num_attributes)
        : new_non_fds_(num_attributes) {}

    /**
     * Adds given column combination to the lifetime storage.
     * If the storage had no such combination, it is added to the last-access storage as well.
     *
     * @param column_set column combination
     */
    void Add(boost::dynamic_bitset<>&& column_set);

    /**
     * @return Number of column sets stored in the last-access storage.
     */
    [[nodiscard]] size_t Count() const {
        return new_non_fds_.GetTotalCount();
    }

    /**
     * @return Collection of violating column sets found since this method previous call.
     * @see NonFDList
     */
    NonFDList MoveOutNewNonFds();

    [[nodiscard]] size_t NumAttributes() const {
        return new_non_fds_.GetNumAttributes();
    }
};

}  // namespace algos::hyfd
