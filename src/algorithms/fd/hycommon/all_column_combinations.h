#pragma once

#include <unordered_set>
#include <vector>

#include <boost/dynamic_bitset.hpp>

#include "column_combination_list.h"

namespace algos::hy {

/**
 * Collection of column combinations found by the Sampler.
 *
 * Stores combinations found during the lifetime as well as combinations found since
 * the last access.
 */
class AllColumnCombinations {
private:
    std::unordered_set<boost::dynamic_bitset<>> total_ccs_;
    ColumnCombinationList new_ccs_;

public:
    explicit AllColumnCombinations(size_t num_attributes) : new_ccs_(num_attributes) {}

    /**
     * Adds given column combination to the lifetime storage.
     * If the storage had no such combination, it is added to the last-access storage as well.
     *
     * @param column_set column combination
     */
    void Add(boost::dynamic_bitset<>&& column_set);
    void Add(boost::dynamic_bitset<> const& column_set);

    /**
     * @return Number of column sets stored in the last-access storage.
     */
    [[nodiscard]] size_t Count() const {
        return new_ccs_.GetTotalCount();
    }

    /**
     * @return Collection of violating column sets found since this method previous call.
     * @see NonFDList
     */
    ColumnCombinationList MoveOutNewColumnCombinations();

    [[nodiscard]] size_t NumAttributes() const {
        return new_ccs_.GetNumAttributes();
    }
};

}  // namespace algos::hy
