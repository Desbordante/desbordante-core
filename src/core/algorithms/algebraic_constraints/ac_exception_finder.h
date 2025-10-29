#pragma once

#include <cstddef>
#include <vector>

#include "ac_exception.h"
#include "ranges_collection.h"

namespace model {
class TypedColumnData;
}  // namespace model

namespace algos {
class ACAlgorithm;
struct RangesCollection;
}  // namespace algos

namespace algos::algebraic_constraints {

class ACExceptionFinder {
private:
    std::vector<ACException> exceptions_;
    ACAlgorithm const* ac_alg_;
    static bool ValueBelongsToRanges(RangesCollection const& ranges_collection,
                                     std::byte const* val);
    /* Creates new ACException and adds it to exceptions_ or adds col_pair to existing one */
    void AddException(size_t row_i, std::pair<size_t, size_t> const& col_pair);
    void CollectColumnPairExceptions(std::vector<model::TypedColumnData> const& data,
                                     RangesCollection const& ranges_collection);

public:
    void CollectExceptions(ACAlgorithm const* ac_alg);

    void ResetState() {
        exceptions_.clear();
    }

    std::vector<ACException> const& GetACExceptions() const {
        return exceptions_;
    }
};

}  // namespace algos::algebraic_constraints
